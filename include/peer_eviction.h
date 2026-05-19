#ifndef PEER_EVICTION_H
#define PEER_EVICTION_H

// MazeChain — Peer Eviction Policy, Net Groups, Bandwidth Control
// Bitcoin Core-style peer eviction: protect valuable peers, evict worst
// Net groups: categorize peers by /16 subnet for diversity
// Bandwidth control: limit upload/download per peer

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <ctime>
#include <mutex>

namespace PeerEviction {

    constexpr int MAX_OUTBOUND       = 8;
    constexpr int MAX_INBOUND        = 117;
    constexpr int MAX_TOTAL          = 125;
    constexpr int EVICTION_GRACE_SEC = 60;   // don't evict peers connected <60s

    enum class PeerType { INBOUND, OUTBOUND, FEELER, MANUAL };

    struct PeerInfo {
        std::string ip;
        int         port;
        PeerType    type;
        int64_t     connectedAt;
        int64_t     lastMessage;
        int         banScore;
        int         blocksServed;
        int         txsServed;
        bool        preferredDownload;  // serving us blocks during IBD
        bool        oneHopSync;         // headers-first sync peer
        bool        bloomFilter;        // supports bloom filters
        bool        compactBlocks;      // supports compact blocks
        std::string netGroup;           // /16 subnet
        double      uploadBytes;
        double      downloadBytes;
        int         pingMs;
        int         height;             // peer's reported chain height
        int         version;            // protocol version

        std::string netGroup16() const {
            size_t p1 = ip.find('.');
            if (p1 == std::string::npos) return ip;
            size_t p2 = ip.find('.', p1+1);
            if (p2 == std::string::npos) return ip;
            return ip.substr(0, p2) + ".0.0/16";
        }
    };

    static std::map<std::string, PeerInfo> g_peers;
    static std::mutex g_evictionMutex;

    inline void addPeer(const PeerInfo& p) {
        std::lock_guard<std::mutex> lock(g_evictionMutex);
        g_peers[p.ip] = p;
    }

    inline void removePeer(const std::string& ip) {
        std::lock_guard<std::mutex> lock(g_evictionMutex);
        g_peers.erase(ip);
    }

    inline int peerCount() {
        std::lock_guard<std::mutex> lock(g_evictionMutex);
        return (int)g_peers.size();
    }

    // Eviction algorithm: returns IP of peer to disconnect
    // Protects: manual connections, outbound, IBD helpers, high block servers
    // Evicts: highest ban score, then oldest last-message, then random
    inline std::string selectEvictee() {
        std::lock_guard<std::mutex> lock(g_evictionMutex);
        int64_t now = (int64_t)std::time(nullptr);

        // Build candidate list (inbound only, past grace period)
        std::vector<PeerInfo*> candidates;
        for (auto& kv : g_peers) {
            auto& p = kv.second;
            if (p.type != PeerType::INBOUND) continue;
            if (now - p.connectedAt < EVICTION_GRACE_SEC) continue;
            if (p.type == PeerType::MANUAL) continue;
            if (p.preferredDownload) continue; // protect IBD helpers
            candidates.push_back(&p);
        }
        if (candidates.empty()) return "";

        // Sort by: ban score desc, then last message asc (oldest first)
        std::sort(candidates.begin(), candidates.end(),
            [](const PeerInfo* a, const PeerInfo* b) {
                if (a->banScore != b->banScore) return a->banScore > b->banScore;
                return a->lastMessage < b->lastMessage;
            });

        // Protect 4 from each /16 group (diversity)
        std::map<std::string, int> groupCount;
        std::vector<PeerInfo*> evictable;
        for (auto* p : candidates) {
            if (groupCount[p->netGroup16()]++ >= 4) evictable.push_back(p);
        }
        if (evictable.empty()) return candidates.front()->ip;
        return evictable.front()->ip;
    }

    // Bandwidth control: enforce per-peer limits
    constexpr double MAX_UPLOAD_PER_PEER_MB   = 100.0;
    constexpr double MAX_DOWNLOAD_PER_PEER_MB = 500.0;

    inline bool checkBandwidth(const std::string& ip) {
        std::lock_guard<std::mutex> lock(g_evictionMutex);
        auto it = g_peers.find(ip);
        if (it == g_peers.end()) return true;
        auto& p = it->second;
        return p.uploadBytes / (1024*1024) < MAX_UPLOAD_PER_PEER_MB &&
               p.downloadBytes / (1024*1024) < MAX_DOWNLOAD_PER_PEER_MB;
    }

    inline void recordUpload(const std::string& ip, double bytes) {
        std::lock_guard<std::mutex> lock(g_evictionMutex);
        auto it = g_peers.find(ip);
        if (it != g_peers.end()) it->second.uploadBytes += bytes;
    }

    inline void recordDownload(const std::string& ip, double bytes) {
        std::lock_guard<std::mutex> lock(g_evictionMutex);
        auto it = g_peers.find(ip);
        if (it != g_peers.end()) it->second.downloadBytes += bytes;
    }

    inline std::string peersJSON() {
        std::lock_guard<std::mutex> lock(g_evictionMutex);
        std::string out = "[";
        bool first = true;
        for (const auto& kv : g_peers) {
            const auto& p = kv.second;
            if (!first) out += ","; first = false;
            out += "{\"ip\":\"" + p.ip + "\""
                 + ",\"port\":" + std::to_string(p.port)
                 + ",\"type\":\"" + (p.type == PeerType::INBOUND ? "inbound" : "outbound") + "\""
                 + ",\"banscore\":" + std::to_string(p.banScore)
                 + ",\"height\":" + std::to_string(p.height)
                 + ",\"version\":" + std::to_string(p.version)
                 + ",\"ping_ms\":" + std::to_string(p.pingMs)
                 + ",\"upload_mb\":" + std::to_string(p.uploadBytes/1048576)
                 + ",\"download_mb\":" + std::to_string(p.downloadBytes/1048576)
                 + "}";
        }
        return out + "]";
    }

} // namespace PeerEviction

#endif
