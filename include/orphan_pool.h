#ifndef ORPHAN_POOL_H
#define ORPHAN_POOL_H

// MazeChain — Orphan Transaction & Block Pool
// Stores transactions/blocks whose parents are not yet known
// Auto-expires orphans after TTL; limits pool size

#include <map>
#include <vector>
#include <string>
#include <ctime>
#include <mutex>
#include <iostream>

namespace OrphanPool {

    // ── Orphan Transaction ─────────────────────────────────────────────────────
    struct OrphanTx {
        std::string txid;
        std::string missingParentTxid;
        std::string rawData;        // serialized tx
        int64_t     receivedTime;
        std::string fromPeer;
    };

    // ── Orphan Block ───────────────────────────────────────────────────────────
    struct OrphanBlock {
        int         index;
        std::string hash;
        std::string prevHash;
        int64_t     receivedTime;
        std::string fromPeer;
    };

    constexpr int    MAX_ORPHAN_TX    = 100;
    constexpr int    MAX_ORPHAN_BLOCK = 50;
    constexpr int64_t ORPHAN_TX_TTL  = 3600;   // 1 hour
    constexpr int64_t ORPHAN_BLK_TTL = 600;    // 10 minutes

    static std::map<std::string, OrphanTx>    g_orphanTxs;
    static std::map<std::string, OrphanBlock> g_orphanBlocks;
    static std::mutex g_orphanMutex;

    // ── Transactions ───────────────────────────────────────────────────────────
    inline void addOrphanTx(const OrphanTx& tx) {
        std::lock_guard<std::mutex> lock(g_orphanMutex);
        if ((int)g_orphanTxs.size() >= MAX_ORPHAN_TX) {
            g_orphanTxs.erase(g_orphanTxs.begin()); // evict oldest
        }
        g_orphanTxs[tx.txid] = tx;
        std::cout << "📦 [ORPHAN] TX " << tx.txid.substr(0,16) << " aguardando pai " << tx.missingParentTxid.substr(0,16) << std::endl;
    }

    inline bool hasOrphanTx(const std::string& txid) {
        std::lock_guard<std::mutex> lock(g_orphanMutex);
        return g_orphanTxs.count(txid) > 0;
    }

    inline void removeOrphanTx(const std::string& txid) {
        std::lock_guard<std::mutex> lock(g_orphanMutex);
        g_orphanTxs.erase(txid);
    }

    // Resolve orphans whose parent just arrived
    inline std::vector<OrphanTx> resolveOrphans(const std::string& parentTxid) {
        std::lock_guard<std::mutex> lock(g_orphanMutex);
        std::vector<OrphanTx> resolved;
        for (auto it = g_orphanTxs.begin(); it != g_orphanTxs.end(); ) {
            if (it->second.missingParentTxid == parentTxid) {
                resolved.push_back(it->second);
                it = g_orphanTxs.erase(it);
            } else ++it;
        }
        return resolved;
    }

    inline int purgeExpiredTxs() {
        std::lock_guard<std::mutex> lock(g_orphanMutex);
        int64_t now = (int64_t)std::time(nullptr);
        int removed = 0;
        for (auto it = g_orphanTxs.begin(); it != g_orphanTxs.end(); ) {
            if (now - it->second.receivedTime > ORPHAN_TX_TTL) {
                it = g_orphanTxs.erase(it); removed++;
            } else ++it;
        }
        return removed;
    }

    inline int orphanTxCount() {
        std::lock_guard<std::mutex> lock(g_orphanMutex);
        return (int)g_orphanTxs.size();
    }

    // ── Blocks ─────────────────────────────────────────────────────────────────
    inline void addOrphanBlock(const OrphanBlock& blk) {
        std::lock_guard<std::mutex> lock(g_orphanMutex);
        if ((int)g_orphanBlocks.size() >= MAX_ORPHAN_BLOCK) {
            g_orphanBlocks.erase(g_orphanBlocks.begin());
        }
        g_orphanBlocks[blk.hash] = blk;
        std::cout << "📦 [ORPHAN BLOCK] #" << blk.index << " aguardando prevHash " << blk.prevHash.substr(0,16) << std::endl;
    }

    inline std::vector<OrphanBlock> resolveOrphanBlocks(const std::string& newBlockHash) {
        std::lock_guard<std::mutex> lock(g_orphanMutex);
        std::vector<OrphanBlock> resolved;
        for (auto it = g_orphanBlocks.begin(); it != g_orphanBlocks.end(); ) {
            if (it->second.prevHash == newBlockHash) {
                resolved.push_back(it->second);
                it = g_orphanBlocks.erase(it);
            } else ++it;
        }
        return resolved;
    }

    inline int purgeExpiredBlocks() {
        std::lock_guard<std::mutex> lock(g_orphanMutex);
        int64_t now = (int64_t)std::time(nullptr);
        int removed = 0;
        for (auto it = g_orphanBlocks.begin(); it != g_orphanBlocks.end(); ) {
            if (now - it->second.receivedTime > ORPHAN_BLK_TTL) {
                it = g_orphanBlocks.erase(it); removed++;
            } else ++it;
        }
        return removed;
    }

    inline int orphanBlockCount() {
        std::lock_guard<std::mutex> lock(g_orphanMutex);
        return (int)g_orphanBlocks.size();
    }

    inline void purgeAll() {
        purgeExpiredTxs();
        purgeExpiredBlocks();
    }

} // namespace OrphanPool

#endif
