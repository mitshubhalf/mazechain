#ifndef STRATUM_H
#define STRATUM_H

// MazeChain — Stratum Protocol (mining pool support)
// JSON-RPC 2.0 over TCP for pool mining communication
// Commands: mining.subscribe, mining.authorize, mining.notify,
//           mining.submit, mining.set_difficulty, client.get_version

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <mutex>
#include <functional>
#include <cstdint>
#include <ctime>

namespace Stratum {

    // ── Session per miner ──────────────────────────────────────────────────────
    struct MinerSession {
        std::string sessionId;
        std::string workerName;
        std::string workerPass;
        double      difficulty    = 1.0;
        int64_t     lastActivity  = 0;
        int         sharesValid   = 0;
        int         sharesInvalid = 0;
        std::string extraNonce1;   // server-assigned 4 bytes hex
        int         extraNonce2Size = 4;
        bool        subscribed    = false;
        bool        authorized    = false;
    };

    // ── Block template for miners ──────────────────────────────────────────────
    struct JobTemplate {
        std::string jobId;
        std::string prevHash;
        std::string coinbase1;     // coinbase part 1 (up to extraNonce)
        std::string coinbase2;     // coinbase part 2 (after extraNonce)
        std::vector<std::string> merkleBranch;
        int         version;
        std::string nbits;         // compact difficulty
        std::string ntime;         // timestamp hex
        bool        cleanJobs;
        int         height;
        double      reward;
        std::string minerAddr;
    };

    // ── JSON-RPC message builders ──────────────────────────────────────────────
    inline std::string makeError(int id, int code, const std::string& msg) {
        std::ostringstream ss;
        ss << "{\"id\":" << id << ",\"result\":null,\"error\":["
           << code << ",\"" << msg << "\",null]}\n";
        return ss.str();
    }

    inline std::string makeResult(int id, const std::string& result) {
        return "{\"id\":" + std::to_string(id) + ",\"result\":" + result + ",\"error\":null}\n";
    }

    inline std::string makeNotify(const JobTemplate& job) {
        std::ostringstream ss;
        ss << "{\"id\":null,\"method\":\"mining.notify\",\"params\":["
           << "\"" << job.jobId << "\","
           << "\"" << job.prevHash << "\","
           << "\"" << job.coinbase1 << "\","
           << "\"" << job.coinbase2 << "\","
           << "[";
        for (size_t i = 0; i < job.merkleBranch.size(); i++) {
            if (i) ss << ",";
            ss << "\"" << job.merkleBranch[i] << "\"";
        }
        ss << "],"
           << "\"" << std::hex << job.version << "\","
           << "\"" << job.nbits << "\","
           << "\"" << job.ntime << "\","
           << (job.cleanJobs ? "true" : "false")
           << "]}\n";
        return ss.str();
    }

    inline std::string makeSetDifficulty(double diff) {
        std::ostringstream ss;
        ss << "{\"id\":null,\"method\":\"mining.set_difficulty\",\"params\":["
           << diff << "]}\n";
        return ss.str();
    }

    // Subscribe response: [session_id, extra_nonce1, extra_nonce2_size]
    inline std::string makeSubscribeResult(int id, const std::string& sessionId,
                                            const std::string& extraNonce1, int en2size) {
        std::ostringstream ss;
        ss << "{\"id\":" << id << ",\"result\":[["
           << "[\"mining.set_difficulty\",\"" << sessionId << "\"],"
           << "[\"mining.notify\",\"" << sessionId << "\"]],"
           << "\"" << extraNonce1 << "\","
           << en2size << "],\"error\":null}\n";
        return ss.str();
    }

    // ── Share submission tracking ──────────────────────────────────────────────
    struct Share {
        std::string workerId;
        std::string jobId;
        std::string extraNonce2;
        std::string ntime;
        std::string nonce;
        bool        valid;
        int64_t     timestamp;
        double      difficulty;
    };

    // ── Pool statistics ────────────────────────────────────────────────────────
    struct PoolStats {
        int64_t startTime  = (int64_t)std::time(nullptr);
        int     totalMiners = 0;
        int     activeMiners= 0;
        double  poolHashrate= 0.0;
        int     sharesValid = 0;
        int     sharesInvalid= 0;
        int     blocksFound = 0;
        double  totalPaid   = 0.0;
    };

    static PoolStats g_stats;
    static std::mutex g_stratumMutex;
    static std::map<std::string, MinerSession> g_sessions;
    static std::map<std::string, JobTemplate>  g_jobs;

    inline std::string newSessionId() {
        static int counter = 0;
        std::ostringstream ss;
        ss << std::hex << (int64_t)std::time(nullptr) << std::hex << (++counter);
        return ss.str();
    }

    inline std::string newExtraNonce1() {
        static int counter = 0;
        std::ostringstream ss;
        ss << std::hex << std::setw(8) << std::setfill('0') << (++counter);
        return ss.str();
    }

    // ExtraNonce: allows pool to give each miner unique search space
    inline std::string buildCoinbase(const std::string& minerAddr, double reward,
                                      const std::string& extraNonce1, int en2Size,
                                      int height) {
        // Coinbase structure: [coinbase1] [extraNonce1] [extraNonce2] [coinbase2]
        std::string coinbase1 = "coinbase_h" + std::to_string(height) + "_reward_";
        std::string coinbase2 = "_miner_" + minerAddr;
        return coinbase1 + extraNonce1 + std::string(en2Size*2, '0') + coinbase2;
    }

    inline MinerSession& createSession() {
        std::lock_guard<std::mutex> lock(g_stratumMutex);
        MinerSession s;
        s.sessionId    = newSessionId();
        s.extraNonce1  = newExtraNonce1();
        s.lastActivity = (int64_t)std::time(nullptr);
        g_sessions[s.sessionId] = s;
        g_stats.totalMiners++;
        return g_sessions[s.sessionId];
    }

    inline void recordShare(const Share& share) {
        std::lock_guard<std::mutex> lock(g_stratumMutex);
        if (share.valid) g_stats.sharesValid++;
        else             g_stats.sharesInvalid++;
        auto it = g_sessions.find(share.workerId);
        if (it != g_sessions.end()) {
            if (share.valid) it->second.sharesValid++;
            else             it->second.sharesInvalid++;
        }
    }

    inline std::string statsJSON() {
        std::lock_guard<std::mutex> lock(g_stratumMutex);
        int64_t uptime = (int64_t)std::time(nullptr) - g_stats.startTime;
        std::ostringstream ss;
        ss << "{\"uptime\":" << uptime
           << ",\"total_miners\":" << g_stats.totalMiners
           << ",\"active_miners\":" << (int)g_sessions.size()
           << ",\"pool_hashrate\":" << g_stats.poolHashrate
           << ",\"shares_valid\":" << g_stats.sharesValid
           << ",\"shares_invalid\":" << g_stats.sharesInvalid
           << ",\"blocks_found\":" << g_stats.blocksFound
           << ",\"total_paid\":" << g_stats.totalPaid
           << "}";
        return ss.str();
    }

} // namespace Stratum

#endif
