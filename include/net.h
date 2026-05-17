#ifndef MAZECHAIN_NET_H
#define MAZECHAIN_NET_H

#include <iostream>
#include <vector>
#include <string>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <optional>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <cassert>
#include <cstring> // Para memcpy seguro

using NodeId = int64_t;

// ============================
// Utils básicos
// ============================

struct NetMessage {
    std::string type;
    std::vector<uint8_t> payload;
    std::chrono::steady_clock::time_point time;

    NetMessage() = default;

    NetMessage(std::string t, std::vector<uint8_t> p)
        : type(std::move(t)), payload(std::move(p)),
          time(std::chrono::steady_clock::now()) {}
};

// ============================
// Transport (estilo Bitcoin)
// ============================

class Transport {
public:
    virtual ~Transport() = default;

    virtual bool ReceiveBytes(const std::vector<uint8_t>& bytes) = 0;
    virtual bool HasMessage() const = 0;
    virtual NetMessage GetMessage() = 0;

    virtual void SendMessage(const NetMessage& msg) = 0;
    virtual std::vector<uint8_t> GetBytesToSend() = 0;
};

// ============================
// Transporte simples (V1-like)
// ============================

class V1Transport : public Transport {
private:
    std::vector<uint8_t> recvBuffer;
    std::deque<NetMessage> messages;
    std::vector<uint8_t> sendBuffer;
    mutable std::mutex mtx; // mutable para permitir HasMessage const

public:
    inline bool ReceiveBytes(const std::vector<uint8_t>& bytes) override {
        std::lock_guard<std::mutex> lock(mtx);
        recvBuffer.insert(recvBuffer.end(), bytes.begin(), bytes.end());

        // protocolo simples: [size][data]
        while (recvBuffer.size() >= 4) {
            uint32_t size;
            std::memcpy(&size, &recvBuffer[0], 4);

            if (recvBuffer.size() < size + 4)
                break;

            std::vector<uint8_t> payload(
                recvBuffer.begin() + 4,
                recvBuffer.begin() + 4 + size
            );

            messages.emplace_back("data", std::move(payload));
            recvBuffer.erase(recvBuffer.begin(), recvBuffer.begin() + 4 + size);
        }
        return true;
    }

    inline bool HasMessage() const override {
        std::lock_guard<std::mutex> lock(mtx);
        return !messages.empty();
    }

    inline NetMessage GetMessage() override {
        std::lock_guard<std::mutex> lock(mtx);
        auto msg = std::move(messages.front());
        messages.pop_front();
        return msg;
    }

    inline void SendMessage(const NetMessage& msg) override {
        std::lock_guard<std::mutex> lock(mtx);
        uint32_t size = static_cast<uint32_t>(msg.payload.size());

        size_t oldSize = sendBuffer.size();
        sendBuffer.resize(oldSize + 4 + size);

        std::memcpy(&sendBuffer[oldSize], &size, 4);
        std::memcpy(&sendBuffer[oldSize + 4], msg.payload.data(), size);
    }

    inline std::vector<uint8_t> GetBytesToSend() override {
        std::lock_guard<std::mutex> lock(mtx);
        std::vector<uint8_t> out = std::move(sendBuffer);
        sendBuffer.clear();
        return out;
    }
};

// ============================
// Node (peer)
// ============================

class MazeNode {
private:
    NodeId id;
    std::unique_ptr<Transport> transport;
    std::mutex sendMutex;
    std::mutex recvMutex;
    std::deque<NetMessage> sendQueue;
    std::deque<NetMessage> recvQueue;
    std::atomic<bool> connected{true};

public:
    MazeNode(NodeId nodeId)
        : id(nodeId),
          transport(std::make_unique<V1Transport>()) {}

    NodeId GetId() const { return id; }

    inline void PushMessage(const NetMessage& msg) {
        std::lock_guard<std::mutex> lock(sendMutex);
        sendQueue.push_back(msg);
    }

    inline void ProcessSend() {
        std::lock_guard<std::mutex> lock(sendMutex);
        while (!sendQueue.empty()) {
            transport->SendMessage(sendQueue.front());
            sendQueue.pop_front();
        }
    }

    inline std::vector<uint8_t> FlushSend() {
        return transport->GetBytesToSend();
    }

    inline void ReceiveBytes(const std::vector<uint8_t>& bytes) {
        transport->ReceiveBytes(bytes);
        while (transport->HasMessage()) {
            std::lock_guard<std::mutex> lock(recvMutex);
            recvQueue.push_back(transport->GetMessage());
        }
    }

    inline std::optional<NetMessage> PopMessage() {
        std::lock_guard<std::mutex> lock(recvMutex);
        if (recvQueue.empty())
            return std::nullopt;

        auto msg = std::move(recvQueue.front());
        recvQueue.pop_front();
        return msg;
    }

    bool IsConnected() const { return connected; }
    void Disconnect() { connected = false; }
};

// ============================
// Interface de eventos
// ============================

class NetEventsInterface {
public:
    virtual ~NetEventsInterface() = default;
    virtual void OnMessage(MazeNode& node, const NetMessage& msg) = 0;
    virtual void OnConnect(MazeNode& node) = 0;
    virtual void OnDisconnect(MazeNode& node) = 0;
};

// ============================
// Connman (gerenciador)
// ============================

class MazeConnman {
private:
    std::map<NodeId, std::shared_ptr<MazeNode>> nodes;
    std::mutex nodesMutex;
    std::atomic<bool> running{false};
    std::thread worker;
    NetEventsInterface* events{nullptr};
    NodeId nextId{1};

public:
    void SetEvents(NetEventsInterface* e) { events = e; }

    inline std::shared_ptr<MazeNode> CreateNode() {
        std::lock_guard<std::mutex> lock(nodesMutex);
        auto node = std::make_shared<MazeNode>(nextId++);
        nodes[node->GetId()] = node;
        if (events) events->OnConnect(*node);
        return node;
    }

    inline void RemoveNode(NodeId id) {
        std::lock_guard<std::mutex> lock(nodesMutex);
        if (nodes.count(id)) {
            if (events) events->OnDisconnect(*nodes[id]);
            nodes.erase(id);
        }
    }

    inline void Start() {
        if (running.exchange(true)) return;
        worker = std::thread([this]() {
            while (running) {
                Process();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }

    inline void Stop() {
        running = false;
        if (worker.joinable())
            worker.join();
    }

    inline void Process() {
        std::lock_guard<std::mutex> lock(nodesMutex);

        for (auto it = nodes.begin(); it != nodes.end(); ) {
            auto& node = it->second;

            if (!node->IsConnected()) {
                if (events) events->OnDisconnect(*node);
                it = nodes.erase(it);
                continue;
            }

            node->ProcessSend();
            auto bytes = node->FlushSend();

            // Simulação de loopback/rede
            if (!bytes.empty()) {
                node->ReceiveBytes(bytes);
            }

            while (true) {
                auto msg = node->PopMessage();
                if (!msg.has_value()) break;
                if (events) {
                    events->OnMessage(*node, msg.value());
                }
            }
            ++it;
        }
    }
};

#endif // MAZECHAIN_NET_H