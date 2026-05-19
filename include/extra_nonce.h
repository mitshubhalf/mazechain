#ifndef EXTRA_NONCE_H
#define EXTRA_NONCE_H

// MazeChain — ExtraNonce, Template Caching, Timestamp Rolling
// ExtraNonce: extra field in coinbase to expand search space beyond nonce
// Template caching: avoid reconstructing templates on every poll
// Timestamp rolling: update timestamp periodically during mining

#include <string>
#include <atomic>
#include <ctime>
#include <mutex>
#include <sstream>
#include <iomanip>

namespace ExtraNonce {

    // ── ExtraNonce state ───────────────────────────────────────────────────────
    static std::atomic<uint32_t> g_extraNonce1{0};
    static std::atomic<uint32_t> g_extraNonce2{0};

    inline uint32_t nextExtraNonce1() { return ++g_extraNonce1; }
    inline uint32_t nextExtraNonce2() { return ++g_extraNonce2; }

    inline std::string extraNonce1Hex() {
        std::ostringstream ss;
        ss << std::hex << std::setw(8) << std::setfill('0') << g_extraNonce1.load();
        return ss.str();
    }

    inline std::string extraNonce2Hex() {
        std::ostringstream ss;
        ss << std::hex << std::setw(8) << std::setfill('0') << g_extraNonce2.load();
        return ss.str();
    }

    // Build coinbase tx with extraNonce embedded
    // This expands the nonce search space to 2^64 effectively
    inline std::string buildCoinbaseTxId(int blockHeight, const std::string& minerAddr,
                                          double reward, uint32_t en1, uint32_t en2) {
        std::ostringstream ss;
        ss << "coinbase"
           << "_h"    << blockHeight
           << "_en1"  << std::hex << en1
           << "_en2"  << std::hex << en2
           << "_r"    << std::fixed << std::setprecision(8) << reward
           << "_"     << minerAddr.substr(0, 20);
        return ss.str();
    }

    // ── Block Template Cache ───────────────────────────────────────────────────
    struct BlockTemplate {
        int         height;
        std::string prevHash;
        int         difficulty;
        double      reward;
        int64_t     createdAt;
        int64_t     timestamp;    // updated via rolling
        std::string minerAddr;
        std::string coinbaseTxId;
        std::string merkleRoot;
        bool        valid;

        bool isStale(int currentHeight, int maxAgeSeconds = 30) const {
            if (height != currentHeight) return true;
            return (int64_t)std::time(nullptr) - createdAt > maxAgeSeconds;
        }

        // Roll timestamp (update without rebuilding full template)
        void rollTimestamp() {
            timestamp = (int64_t)std::time(nullptr);
        }
    };

    static BlockTemplate g_cachedTemplate;
    static std::mutex    g_templateMutex;

    inline void cacheTemplate(const BlockTemplate& tmpl) {
        std::lock_guard<std::mutex> lock(g_templateMutex);
        g_cachedTemplate = tmpl;
    }

    inline BlockTemplate getCachedTemplate() {
        std::lock_guard<std::mutex> lock(g_templateMutex);
        return g_cachedTemplate;
    }

    inline void invalidateTemplate() {
        std::lock_guard<std::mutex> lock(g_templateMutex);
        g_cachedTemplate.valid = false;
    }

    // Timestamp rolling: update timestamp in template every N seconds
    inline void rollingTimestamp(int intervalSec = 10) {
        std::lock_guard<std::mutex> lock(g_templateMutex);
        int64_t now = (int64_t)std::time(nullptr);
        if (now - g_cachedTemplate.timestamp >= intervalSec) {
            g_cachedTemplate.timestamp = now;
        }
    }

    // ASIC-friendly: ensure nonce space is uniformly distributed
    // Returns initial nonce for a given thread (parallel mining)
    inline uint32_t threadStartNonce(int threadId, int totalThreads) {
        return (uint32_t)(((uint64_t)0xFFFFFFFF / totalThreads) * threadId);
    }

    inline uint32_t threadEndNonce(int threadId, int totalThreads) {
        return (uint32_t)(((uint64_t)0xFFFFFFFF / totalThreads) * (threadId + 1));
    }

} // namespace ExtraNonce

#endif
