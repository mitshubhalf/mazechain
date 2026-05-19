#ifndef METRICS_H
#define METRICS_H

// MazeChain — Prometheus-compatible Metrics & Admin Console
// Exposes /metrics endpoint in text/plain format
// Tracks: blocks, txs, peers, mempool, hashrate, supply, fees

#include <map>
#include <string>
#include <sstream>
#include <mutex>
#include <atomic>
#include <ctime>
#include <deque>
#include <numeric>
#include <algorithm>

namespace Metrics {

    // ── Counters & Gauges ──────────────────────────────────────────────────────
    static std::atomic<uint64_t> blocks_mined{0};
    static std::atomic<uint64_t> txs_total{0};
    static std::atomic<uint64_t> txs_mempool{0};
    static std::atomic<uint64_t> orphan_txs{0};
    static std::atomic<uint64_t> orphan_blocks{0};
    static std::atomic<uint64_t> peers_connected{0};
    static std::atomic<uint64_t> peers_banned{0};
    static std::atomic<uint64_t> api_requests{0};
    static std::atomic<uint64_t> api_errors{0};
    static std::atomic<uint64_t> dos_blocked{0};

    // Floating point gauges (use mutex)
    static std::mutex g_metricsMutex;
    static double g_hashrate       = 0.0;
    static double g_difficulty     = 4.0;
    static double g_totalSupply    = 0.0;
    static double g_blockReward    = 100.0;
    static double g_avgFeeRate     = 0.0;
    static int64_t g_startTime     = (int64_t)std::time(nullptr);

    // Hashrate history (samples for chart)
    static std::deque<double> g_hashrateHistory;

    inline void setHashrate(double h) {
        std::lock_guard<std::mutex> lock(g_metricsMutex);
        g_hashrate = h;
        g_hashrateHistory.push_back(h);
        if (g_hashrateHistory.size() > 60) g_hashrateHistory.pop_front();
    }

    inline void setDifficulty(double d) {
        std::lock_guard<std::mutex> lock(g_metricsMutex); g_difficulty = d;
    }

    inline void setSupply(double s) {
        std::lock_guard<std::mutex> lock(g_metricsMutex); g_totalSupply = s;
    }

    inline void setBlockReward(double r) {
        std::lock_guard<std::mutex> lock(g_metricsMutex); g_blockReward = r;
    }

    inline void setAvgFeeRate(double f) {
        std::lock_guard<std::mutex> lock(g_metricsMutex); g_avgFeeRate = f;
    }

    inline void incBlocksMined()   { blocks_mined++; }
    inline void incTxs()           { txs_total++; }
    inline void incApiRequest()    { api_requests++; }
    inline void incApiError()      { api_errors++; }
    inline void incDosBlocked()    { dos_blocked++; }

    inline void setMempool(int n)  { txs_mempool.store(n); }
    inline void setPeers(int n)    { peers_connected.store(n); }
    inline void setBanned(int n)   { peers_banned.store(n); }
    inline void setOrphanTx(int n) { orphan_txs.store(n); }
    inline void setOrphanBlk(int n){ orphan_blocks.store(n); }

    // ── Prometheus text format ──────────────────────────────────────────────────
    inline std::string prometheus() {
        std::lock_guard<std::mutex> lock(g_metricsMutex);
        int64_t uptime = (int64_t)std::time(nullptr) - g_startTime;
        std::ostringstream ss;

        auto gauge = [&](const std::string& name, const std::string& help,
                         const std::string& type, double value,
                         const std::string& labels = "") {
            ss << "# HELP " << name << " " << help << "\n";
            ss << "# TYPE " << name << " " << type << "\n";
            ss << name;
            if (!labels.empty()) ss << "{" << labels << "}";
            ss << " " << value << "\n";
        };

        gauge("mazechain_uptime_seconds",    "Node uptime in seconds", "gauge", (double)uptime);
        gauge("mazechain_blocks_total",      "Total blocks mined",     "counter", (double)blocks_mined.load());
        gauge("mazechain_txs_total",         "Total transactions",     "counter", (double)txs_total.load());
        gauge("mazechain_mempool_size",      "Mempool transaction count","gauge", (double)txs_mempool.load());
        gauge("mazechain_peers_connected",   "Connected peers",        "gauge", (double)peers_connected.load());
        gauge("mazechain_peers_banned",      "Banned peers",           "gauge", (double)peers_banned.load());
        gauge("mazechain_hashrate",          "Estimated hash/s",       "gauge", g_hashrate);
        gauge("mazechain_difficulty",        "Current PoW difficulty", "gauge", g_difficulty);
        gauge("mazechain_total_supply",      "Total MZ in circulation","gauge", g_totalSupply);
        gauge("mazechain_block_reward",      "Current block reward MZ","gauge", g_blockReward);
        gauge("mazechain_avg_fee_rate",      "Average fee rate MZ/byte","gauge", g_avgFeeRate);
        gauge("mazechain_api_requests_total","Total API requests",     "counter", (double)api_requests.load());
        gauge("mazechain_api_errors_total",  "Total API errors",       "counter", (double)api_errors.load());
        gauge("mazechain_dos_blocked_total", "DoS blocked requests",   "counter", (double)dos_blocked.load());
        gauge("mazechain_orphan_txs",        "Orphan transactions",    "gauge", (double)orphan_txs.load());
        gauge("mazechain_orphan_blocks",     "Orphan blocks",          "gauge", (double)orphan_blocks.load());

        return ss.str();
    }

    // ── JSON format for admin console ──────────────────────────────────────────
    inline std::string toJSON() {
        std::lock_guard<std::mutex> lock(g_metricsMutex);
        int64_t uptime = (int64_t)std::time(nullptr) - g_startTime;

        // hashrate history as array
        std::string hrHistory = "[";
        for (size_t i = 0; i < g_hashrateHistory.size(); i++) {
            if (i) hrHistory += ",";
            hrHistory += std::to_string(g_hashrateHistory[i]);
        }
        hrHistory += "]";

        double avgHR = 0;
        if (!g_hashrateHistory.empty()) {
            avgHR = std::accumulate(g_hashrateHistory.begin(), g_hashrateHistory.end(), 0.0)
                    / g_hashrateHistory.size();
        }

        std::ostringstream ss;
        ss << "{";
        ss << "\"uptime\":" << uptime << ",";
        ss << "\"blocks_mined\":" << blocks_mined.load() << ",";
        ss << "\"txs_total\":" << txs_total.load() << ",";
        ss << "\"mempool_size\":" << txs_mempool.load() << ",";
        ss << "\"peers_connected\":" << peers_connected.load() << ",";
        ss << "\"peers_banned\":" << peers_banned.load() << ",";
        ss << "\"hashrate\":" << g_hashrate << ",";
        ss << "\"hashrate_avg\":" << avgHR << ",";
        ss << "\"hashrate_history\":" << hrHistory << ",";
        ss << "\"difficulty\":" << g_difficulty << ",";
        ss << "\"total_supply\":" << g_totalSupply << ",";
        ss << "\"block_reward\":" << g_blockReward << ",";
        ss << "\"avg_fee_rate\":" << g_avgFeeRate << ",";
        ss << "\"api_requests\":" << api_requests.load() << ",";
        ss << "\"api_errors\":" << api_errors.load() << ",";
        ss << "\"dos_blocked\":" << dos_blocked.load() << ",";
        ss << "\"orphan_txs\":" << orphan_txs.load() << ",";
        ss << "\"orphan_blocks\":" << orphan_blocks.load();
        ss << "}";
        return ss.str();
    }

} // namespace Metrics

#endif
