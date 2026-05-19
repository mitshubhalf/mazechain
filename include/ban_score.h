#ifndef BAN_SCORE_H
#define BAN_SCORE_H

// MazeChain — Peer Ban Score, Eviction, DoS Protection
// Ban score accumulates per-IP; banning at 100 points for 24h
// Sybil resistance: limit connections per /24 subnet

#include <map>
#include <string>
#include <ctime>
#include <mutex>
#include <iostream>
#include <vector>
#include <algorithm>

namespace BanScore {

    constexpr int    BAN_THRESHOLD       = 100;
    constexpr int64_t BAN_DURATION_SEC   = 86400;  // 24h
    constexpr int    MAX_CONN_PER_SUBNET = 3;       // Sybil resistance

    // Reasons with their penalty scores
    enum class Reason {
        INVALID_BLOCK         = 100, // immediate ban
        INVALID_HEADER        = 50,
        INVALID_TX            = 10,
        PROTOCOL_VIOLATION    = 20,
        ORPHAN_TX_FLOOD       = 5,
        GETDATA_FLOOD         = 5,
        UNKNOWN_MSG           = 1,
        DOUBLE_SPEND_ATTEMPT  = 80,
        TIMEOFFSET_VIOLATION  = 30,
        VERSION_VIOLATION     = 20,
    };

    struct PeerRecord {
        int     score        = 0;
        int64_t bannedUntil  = 0;
        int     violations   = 0;
        std::string lastReason;
    };

    static std::map<std::string, PeerRecord> g_peers;
    static std::mutex g_banMutex;

    inline std::string getSubnet(const std::string& ip) {
        size_t pos = ip.rfind('.');
        if (pos == std::string::npos) return ip;
        return ip.substr(0, pos) + ".0/24";
    }

    inline void addScore(const std::string& ip, int points, const std::string& reason = "") {
        std::lock_guard<std::mutex> lock(g_banMutex);
        auto& rec = g_peers[ip];
        rec.score += points;
        rec.violations++;
        rec.lastReason = reason;
        if (rec.score >= BAN_THRESHOLD) {
            rec.bannedUntil = (int64_t)std::time(nullptr) + BAN_DURATION_SEC;
            std::cout << "🚫 [BAN] Peer " << ip << " banido por " << (BAN_DURATION_SEC/3600) << "h. Razão: " << reason << std::endl;
        }
    }

    inline void addScore(const std::string& ip, Reason r) {
        addScore(ip, (int)r, "violation");
    }

    inline bool isBanned(const std::string& ip) {
        std::lock_guard<std::mutex> lock(g_banMutex);
        auto it = g_peers.find(ip);
        if (it == g_peers.end()) return false;
        int64_t now = (int64_t)std::time(nullptr);
        if (it->second.bannedUntil > now) return true;
        if (it->second.bannedUntil > 0 && it->second.bannedUntil <= now) {
            it->second.bannedUntil = 0;
            it->second.score = 0;
        }
        return false;
    }

    inline void ban(const std::string& ip, int64_t durationSec = BAN_DURATION_SEC) {
        std::lock_guard<std::mutex> lock(g_banMutex);
        auto& rec = g_peers[ip];
        rec.bannedUntil = (int64_t)std::time(nullptr) + durationSec;
        rec.score = BAN_THRESHOLD;
        std::cout << "🚫 [BAN MANUAL] " << ip << " banido por " << (durationSec/3600) << "h." << std::endl;
    }

    inline void unban(const std::string& ip) {
        std::lock_guard<std::mutex> lock(g_banMutex);
        auto it = g_peers.find(ip);
        if (it != g_peers.end()) { it->second.bannedUntil = 0; it->second.score = 0; }
    }

    inline int getScore(const std::string& ip) {
        std::lock_guard<std::mutex> lock(g_banMutex);
        auto it = g_peers.find(ip);
        return (it != g_peers.end()) ? it->second.score : 0;
    }

    inline std::vector<std::pair<std::string,int64_t>> getBannedList() {
        std::lock_guard<std::mutex> lock(g_banMutex);
        std::vector<std::pair<std::string,int64_t>> list;
        int64_t now = (int64_t)std::time(nullptr);
        for (const auto& kv : g_peers) {
            if (kv.second.bannedUntil > now) list.push_back({kv.first, kv.second.bannedUntil});
        }
        return list;
    }

    // Sybil resistance: count connections per /24 subnet
    inline int subnetConnectionCount(const std::string& ip,
                                     const std::vector<std::string>& connected) {
        std::string subnet = getSubnet(ip);
        int count = 0;
        for (const auto& peer : connected) {
            if (getSubnet(peer) == subnet) count++;
        }
        return count;
    }

    inline bool isSubnetLimited(const std::string& ip,
                                 const std::vector<std::string>& connected) {
        return subnetConnectionCount(ip, connected) >= MAX_CONN_PER_SUBNET;
    }

    inline void expireBans() {
        std::lock_guard<std::mutex> lock(g_banMutex);
        int64_t now = (int64_t)std::time(nullptr);
        for (auto& kv : g_peers) {
            if (kv.second.bannedUntil > 0 && kv.second.bannedUntil <= now) {
                kv.second.bannedUntil = 0;
                kv.second.score = std::max(0, kv.second.score - 10);
            }
        }
    }

} // namespace BanScore

#endif
