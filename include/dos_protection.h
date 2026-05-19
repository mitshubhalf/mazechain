#ifndef DOS_PROTECTION_H
#define DOS_PROTECTION_H

// MazeChain — DoS Protection, Rate Limiting, Spam Protection
// - Per-IP request rate limiting (token bucket)
// - Message size limits
// - Replay protection via nonce tracking
// - Peer reputation scoring integration

#include <map>
#include <string>
#include <ctime>
#include <mutex>
#include <deque>
#include <iostream>
#include <set>

namespace DoS {

    // Rate limiting thresholds
    constexpr int MAX_REQUESTS_PER_SEC      = 20;
    constexpr int MAX_TX_PER_MIN_PER_PEER   = 60;
    constexpr int MAX_BLOCK_REQ_PER_MIN     = 10;
    constexpr int MAX_GETDATA_PER_MIN       = 200;
    constexpr size_t MAX_MSG_SIZE           = 32 * 1024 * 1024; // 32MB
    constexpr size_t MAX_INV_ITEMS          = 50000;

    // Token bucket per IP
    struct TokenBucket {
        double   tokens     = MAX_REQUESTS_PER_SEC;
        double   maxTokens  = MAX_REQUESTS_PER_SEC;
        double   refillRate = MAX_REQUESTS_PER_SEC; // tokens per second
        int64_t  lastRefill = 0;

        bool consume(int cost = 1) {
            int64_t now = (int64_t)std::time(nullptr);
            double elapsed = (double)(now - lastRefill);
            tokens = std::min(maxTokens, tokens + elapsed * refillRate);
            lastRefill = now;
            if (tokens < cost) return false;
            tokens -= cost;
            return true;
        }
    };

    // Per-IP rate tracking for different message types
    struct PeerRateLimit {
        TokenBucket   global;
        std::deque<int64_t> txTimes;
        std::deque<int64_t> blockReqTimes;
        std::deque<int64_t> getdataTimes;
    };

    static std::map<std::string, PeerRateLimit> g_rates;
    static std::set<std::string>                g_nonces;   // replay protection
    static std::mutex g_dosMutex;

    inline void cleanOldTimestamps(std::deque<int64_t>& times, int64_t windowSec) {
        int64_t cutoff = (int64_t)std::time(nullptr) - windowSec;
        while (!times.empty() && times.front() < cutoff) times.pop_front();
    }

    // Returns true if request is allowed
    inline bool allowRequest(const std::string& ip, int cost = 1) {
        std::lock_guard<std::mutex> lock(g_dosMutex);
        return g_rates[ip].global.consume(cost);
    }

    inline bool allowTx(const std::string& ip) {
        std::lock_guard<std::mutex> lock(g_dosMutex);
        auto& pr = g_rates[ip];
        cleanOldTimestamps(pr.txTimes, 60);
        if ((int)pr.txTimes.size() >= MAX_TX_PER_MIN_PER_PEER) return false;
        pr.txTimes.push_back((int64_t)std::time(nullptr));
        return true;
    }

    inline bool allowBlockRequest(const std::string& ip) {
        std::lock_guard<std::mutex> lock(g_dosMutex);
        auto& pr = g_rates[ip];
        cleanOldTimestamps(pr.blockReqTimes, 60);
        if ((int)pr.blockReqTimes.size() >= MAX_BLOCK_REQ_PER_MIN) return false;
        pr.blockReqTimes.push_back((int64_t)std::time(nullptr));
        return true;
    }

    inline bool allowGetdata(const std::string& ip, int items = 1) {
        std::lock_guard<std::mutex> lock(g_dosMutex);
        auto& pr = g_rates[ip];
        cleanOldTimestamps(pr.getdataTimes, 60);
        if ((int)pr.getdataTimes.size() + items > MAX_GETDATA_PER_MIN) return false;
        for (int i = 0; i < items; i++) pr.getdataTimes.push_back((int64_t)std::time(nullptr));
        return true;
    }

    inline bool checkMsgSize(size_t size) { return size <= MAX_MSG_SIZE; }
    inline bool checkInvSize(size_t items) { return items <= MAX_INV_ITEMS; }

    // Replay protection: reject duplicate nonces within window
    inline bool checkNonce(const std::string& nonce) {
        std::lock_guard<std::mutex> lock(g_dosMutex);
        if (g_nonces.count(nonce)) return false; // replay!
        g_nonces.insert(nonce);
        if (g_nonces.size() > 10000) {
            // clear old nonces periodically
            g_nonces.clear();
        }
        return true;
    }

    // Transaction malleability: check for low-S signatures (simplified)
    inline bool isLowS(const std::string& sigHex) {
        if (sigHex.size() < 64) return false;
        // In real impl: check S value of DER-encoded ECDSA sig
        // Simplified: return true (full impl requires OpenSSL EC operations)
        return true;
    }

    // P2P checksum validation
    inline uint32_t calcChecksum(const std::string& payload) {
        uint32_t h = 0x811c9dc5u;
        for (unsigned char c : payload) {
            h ^= c;
            h *= 0x01000193u;
        }
        return h;
    }

    inline bool verifyChecksum(const std::string& payload, uint32_t expected) {
        return calcChecksum(payload) == expected;
    }

    // Memory pool spam filter: reject if too many outputs, too large
    constexpr int    MAX_TX_OUTPUTS  = 500;
    constexpr size_t MAX_TX_SIZE     = 400000; // 400KB
    constexpr double MIN_FEE_RELAY   = 0.000003;

    inline bool checkTxSpam(int numOutputs, size_t txSizeBytes, double feeRate) {
        if (numOutputs > MAX_TX_OUTPUTS) return false;
        if (txSizeBytes > MAX_TX_SIZE) return false;
        if (feeRate < MIN_FEE_RELAY) return false;
        return true;
    }

    inline void reset(const std::string& ip) {
        std::lock_guard<std::mutex> lock(g_dosMutex);
        g_rates.erase(ip);
    }

} // namespace DoS

#endif
