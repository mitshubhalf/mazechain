#ifndef INVENTORY_H
#define INVENTORY_H

// MazeChain — P2P Inventory Protocol (inv/getdata/notfound)
// Bitcoin-compatible inventory messages for blocks and transactions
// Headers-first sync, compact block relay, addr relay

#include <string>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <ctime>
#include <sstream>

namespace Inventory {

    enum class Type : uint32_t {
        ERROR              = 0,
        TX                 = 1,
        BLOCK              = 2,
        FILTERED_BLOCK     = 3,
        CMPCT_BLOCK        = 4,  // compact blocks
        WITNESS_TX         = 0x40000001,
        WITNESS_BLOCK      = 0x40000002,
    };

    struct InvItem {
        Type        type;
        std::string hash;

        std::string typeName() const {
            switch(type) {
                case Type::TX:             return "TX";
                case Type::BLOCK:          return "BLOCK";
                case Type::CMPCT_BLOCK:    return "CMPCT_BLOCK";
                case Type::FILTERED_BLOCK: return "FILTERED_BLOCK";
                case Type::WITNESS_TX:     return "WITNESS_TX";
                case Type::WITNESS_BLOCK:  return "WITNESS_BLOCK";
                default:                   return "UNKNOWN";
            }
        }

        std::string serialize() const {
            return typeName() + ":" + hash;
        }
    };

    // Inventory message: announced items
    struct InvMsg { std::vector<InvItem> items; };

    // GetData message: request specific items
    struct GetDataMsg { std::vector<InvItem> items; };

    // NotFound message: items we don't have
    struct NotFoundMsg { std::vector<InvItem> items; };

    // ── Block Header (for headers-first sync) ─────────────────────────────────
    struct BlockHeader {
        int         version;
        std::string hash;
        std::string prevHash;
        std::string merkleRoot;
        int64_t     timestamp;
        int         difficulty;
        uint64_t    nonce;
        int         height;
        int         txCount;   // number of transactions (not in raw header, aux field)

        std::string serialize() const {
            std::ostringstream ss;
            ss << version << "|" << hash << "|" << prevHash << "|"
               << merkleRoot << "|" << timestamp << "|" << difficulty << "|" << nonce;
            return ss.str();
        }
    };

    // ── Headers message (up to 2000 headers) ──────────────────────────────────
    struct HeadersMsg {
        std::vector<BlockHeader> headers;
        static constexpr int MAX_HEADERS = 2000;
    };

    // ── Compact Block (BIP152) ─────────────────────────────────────────────────
    struct CompactBlock {
        BlockHeader     header;
        uint64_t        nonce;
        std::vector<std::string> shortids;      // 6-byte tx short IDs
        std::vector<std::string> prefilledTxs;  // full txs (coinbase etc)

        std::string serialize() const {
            std::ostringstream ss;
            ss << header.hash << "|" << nonce << "|" << shortids.size();
            return ss.str();
        }
    };

    // ── Peer announcement ──────────────────────────────────────────────────────
    struct AddrRecord {
        std::string ip;
        int         port;
        int64_t     lastSeen;
        int         services;  // bit flags: 1=NETWORK, 4=BLOOM, 8=WITNESS, 1024=COMPACT

        std::string serialize() const {
            return ip + ":" + std::to_string(port) + ":" + std::to_string(lastSeen);
        }
    };

    struct AddrMsg { std::vector<AddrRecord> addrs; static constexpr int MAX = 1000; };

    // ── Inventory announcement tracker ────────────────────────────────────────
    // Avoid re-announcing same item to same peer
    static std::map<std::string, std::set<std::string>> g_announced; // peer->hashes
    static std::mutex g_invMutex;

    inline bool shouldAnnounce(const std::string& peer, const std::string& hash) {
        std::lock_guard<std::mutex> lock(g_invMutex);
        return g_announced[peer].insert(hash).second;
    }

    inline void clearPeer(const std::string& peer) {
        std::lock_guard<std::mutex> lock(g_invMutex);
        g_announced.erase(peer);
    }

    // ── Peer version/handshake ─────────────────────────────────────────────────
    constexpr int   PROTOCOL_VERSION = 70015;
    constexpr int   MIN_PEER_PROTO   = 70001;

    struct VersionMsg {
        int         version     = PROTOCOL_VERSION;
        uint64_t    services    = 1; // NODE_NETWORK
        int64_t     timestamp   = (int64_t)std::time(nullptr);
        std::string addrRecv;
        std::string addrFrom;
        uint64_t    nonce;
        std::string userAgent   = "/MazeChain:4.0.0/";
        int         startHeight = 0;
        bool        relay       = true;
    };

    // ── Initial Block Download tracking ───────────────────────────────────────
    struct IBDState {
        bool   active       = false;
        int    startHeight  = 0;
        int    targetHeight = 0;
        int    currentHeight= 0;
        double progress     = 0.0;
        std::string bestPeer;
        int64_t startTime   = 0;

        double rate() const {
            int64_t elapsed = (int64_t)std::time(nullptr) - startTime;
            if (elapsed <= 0) return 0;
            return (double)(currentHeight - startHeight) / elapsed;
        }

        std::string status() const {
            std::ostringstream ss;
            ss << "IBD: " << currentHeight << "/" << targetHeight
               << " (" << (int)(progress*100) << "%) "
               << rate() << " blk/s";
            return ss.str();
        }
    };

    static IBDState g_ibd;
    inline void startIBD(int from, int to, const std::string& peer) {
        g_ibd = {true, from, to, from, 0.0, peer, (int64_t)std::time(nullptr)};
    }
    inline void updateIBD(int current) {
        g_ibd.currentHeight = current;
        if (g_ibd.targetHeight > g_ibd.startHeight)
            g_ibd.progress = (double)(current - g_ibd.startHeight) /
                             (g_ibd.targetHeight - g_ibd.startHeight);
        if (current >= g_ibd.targetHeight) g_ibd.active = false;
    }
    inline const IBDState& ibdState() { return g_ibd; }

} // namespace Inventory

#endif
