#ifndef SSE_H
#define SSE_H

// MazeChain — Server-Sent Events (SSE) engine
// Thread-safe event queue. Crow handlers long-poll up to 30s waiting for new
// events; the frontend uses EventSource which auto-reconnects via Last-Event-ID.

#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <sstream>
#include <ctime>

namespace SSE {

    enum class Type { BLOCK, TX, PEER, MINING_START, MINING_DONE, NODE_STATUS };

    struct Event {
        uint64_t    id;
        Type        type;
        std::string data;   // JSON string
        int64_t     ts;

        std::string typeName() const {
            switch(type) {
                case Type::BLOCK:        return "block";
                case Type::TX:           return "tx";
                case Type::PEER:         return "peer";
                case Type::MINING_START: return "mining_start";
                case Type::MINING_DONE:  return "mining_done";
                case Type::NODE_STATUS:  return "node_status";
            }
            return "event";
        }

        // Format as SSE wire format
        std::string toSSE() const {
            std::ostringstream ss;
            ss << "id: "    << id         << "\n"
               << "event: " << typeName() << "\n"
               << "data: "  << data       << "\n\n";
            return ss.str();
        }
    };

    // ── Global event bus ───────────────────────────────────────────────────────
    constexpr int MAX_EVENTS = 500;

    static std::vector<Event>        g_events;
    static std::mutex                g_mutex;
    static std::condition_variable   g_cv;
    static std::atomic<uint64_t>     g_nextId{1};

    inline uint64_t push(Type type, const std::string& jsonData) {
        std::unique_lock<std::mutex> lock(g_mutex);
        uint64_t id = g_nextId++;
        Event e{id, type, jsonData, (int64_t)std::time(nullptr)};
        g_events.push_back(e);
        if ((int)g_events.size() > MAX_EVENTS)
            g_events.erase(g_events.begin());
        lock.unlock();
        g_cv.notify_all();
        return id;
    }

    // Wait up to timeoutMs for any event with id > afterId.
    // Returns the matching events as SSE wire text.
    inline std::string waitForEvents(uint64_t afterId, int timeoutMs = 25000) {
        std::unique_lock<std::mutex> lock(g_mutex);

        auto hasNew = [&]() {
            return !g_events.empty() && g_events.back().id > afterId;
        };

        if (!hasNew()) {
            g_cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), hasNew);
        }

        std::string out;
        for (const auto& e : g_events) {
            if (e.id > afterId) out += e.toSSE();
        }
        // keepalive comment so browser doesn't time out
        if (out.empty()) out = ": keepalive\n\n";
        return out;
    }

    // Get recent events as SSE text (for initial page load)
    inline std::string recentEvents(int n = 20) {
        std::lock_guard<std::mutex> lock(g_mutex);
        std::string out;
        int start = std::max(0, (int)g_events.size() - n);
        for (int i = start; i < (int)g_events.size(); i++)
            out += g_events[i].toSSE();
        return out;
    }

    inline uint64_t latestId() {
        std::lock_guard<std::mutex> lock(g_mutex);
        return g_events.empty() ? 0 : g_events.back().id;
    }

    // ── Convenience pushers ────────────────────────────────────────────────────
    inline void pushBlock(int height, const std::string& hash, const std::string& miner,
                          double reward, int txCount, double supply, int difficulty) {
        std::ostringstream ss;
        ss << "{\"height\":" << height
           << ",\"hash\":\""  << hash   << "\""
           << ",\"miner\":\"" << miner  << "\""
           << ",\"reward\":"  << reward
           << ",\"tx_count\":" << txCount
           << ",\"supply\":"  << supply
           << ",\"difficulty\":" << difficulty
           << ",\"ts\":"      << (int64_t)std::time(nullptr) << "}";
        push(Type::BLOCK, ss.str());
    }

    inline void pushMiningStart(int height, const std::string& miner) {
        std::ostringstream ss;
        ss << "{\"height\":" << height << ",\"miner\":\"" << miner << "\","
           << "\"ts\":" << (int64_t)std::time(nullptr) << "}";
        push(Type::MINING_START, ss.str());
    }

    inline void pushMiningDone(int height, const std::string& hash,
                               bool success, double ms) {
        std::ostringstream ss;
        ss << "{\"height\":" << height << ",\"hash\":\"" << hash << "\","
           << "\"success\":" << (success?"true":"false")
           << ",\"ms\":" << ms
           << ",\"ts\":" << (int64_t)std::time(nullptr) << "}";
        push(Type::MINING_DONE, ss.str());
    }

    inline void pushPeer(const std::string& ip, bool connected) {
        std::ostringstream ss;
        ss << "{\"ip\":\"" << ip << "\",\"connected\":" << (connected?"true":"false")
           << ",\"ts\":" << (int64_t)std::time(nullptr) << "}";
        push(Type::PEER, ss.str());
    }

    inline void pushNodeStatus(int height, double supply, int peers,
                               int mempool, double difficulty) {
        std::ostringstream ss;
        ss << "{\"height\":" << height << ",\"supply\":" << supply
           << ",\"peers\":" << peers << ",\"mempool\":" << mempool
           << ",\"difficulty\":" << difficulty
           << ",\"ts\":" << (int64_t)std::time(nullptr) << "}";
        push(Type::NODE_STATUS, ss.str());
    }

} // namespace SSE

#endif
