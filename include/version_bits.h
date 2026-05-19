#ifndef VERSION_BITS_H
#define VERSION_BITS_H

// MazeChain — Version Bits (BIP9) Soft Fork Signaling
// Block version bits: bit 0-28 = feature flags, bits 29-31 = 001 prefix
// Deployment states: DEFINED → STARTED → LOCKED_IN → ACTIVE → FAILED

#include <string>
#include <map>
#include <vector>
#include <ctime>
#include <iostream>

namespace VersionBits {

    // Feature bits (bit position in nVersion)
    enum class Bit : int {
        SEGWIT           = 1,  // BIP141
        TAPROOT          = 2,  // BIP340/341/342
        SCHNORR          = 3,  // BIP340 Schnorr
        CSV              = 4,  // BIP68/112 CSV
        CLTV             = 5,  // BIP65 CLTV
        RBF              = 6,  // BIP125 RBF
        SEGWIT_V2        = 7,  // Future SegWit extension
        COVENANTS        = 8,  // Covenant opcodes
        VAULTS           = 9,  // Vault support
        DLCS             = 10, // Discreet Log Contracts
    };

    enum class State { DEFINED, STARTED, LOCKED_IN, ACTIVE, FAILED };

    struct Deployment {
        std::string name;
        Bit         bit;
        int64_t     startTime;    // UNIX timestamp when signaling starts
        int64_t     timeout;      // UNIX timestamp when deployment expires
        int         threshold;    // blocks needed to lock in (e.g., 1815 of 2016)
        int         period;       // signaling period in blocks (e.g., 2016)
        State       state         = State::DEFINED;
        int         sinceBlock    = 0;
        int         signalingCount= 0;
    };

    static std::map<std::string, Deployment> g_deployments;

    inline void registerDeployment(const Deployment& d) {
        g_deployments[d.name] = d;
        std::cout << "🔧 [VERSION BITS] Deployment '" << d.name << "' registrado (bit " << (int)d.bit << ")" << std::endl;
    }

    inline void initDefault(int64_t now = (int64_t)std::time(nullptr)) {
        registerDeployment({"segwit",   Bit::SEGWIT,   now, now+63072000, 1815, 2016, State::ACTIVE});
        registerDeployment({"taproot",  Bit::TAPROOT,  now, now+63072000, 1815, 2016, State::ACTIVE});
        registerDeployment({"schnorr",  Bit::SCHNORR,  now, now+63072000, 1512, 2016, State::ACTIVE});
        registerDeployment({"csv",      Bit::CSV,      now, now+63072000, 1815, 2016, State::ACTIVE});
        registerDeployment({"cltv",     Bit::CLTV,     now, now+63072000, 1815, 2016, State::ACTIVE});
        registerDeployment({"rbf",      Bit::RBF,      now, now+63072000, 1008, 2016, State::ACTIVE});
        registerDeployment({"covenants",Bit::COVENANTS,now+31536000, now+94608000, 1815, 2016, State::DEFINED});
        registerDeployment({"vaults",   Bit::VAULTS,   now+31536000, now+94608000, 1815, 2016, State::DEFINED});
    }

    // Build nVersion with all active signaling bits
    inline uint32_t buildVersion(int64_t blockTime) {
        uint32_t version = 0x20000000; // top 3 bits = 001
        for (const auto& kv : g_deployments) {
            const auto& d = kv.second;
            if (d.state == State::STARTED &&
                blockTime >= d.startTime && blockTime < d.timeout) {
                version |= (1u << (int)d.bit);
            }
        }
        return version;
    }

    // Process a mined block's version — update signaling counts
    inline void processBlock(int height, uint32_t blockVersion, int64_t blockTime) {
        for (auto& kv : g_deployments) {
            auto& d = kv.second;
            if (d.state == State::FAILED || d.state == State::ACTIVE) continue;

            // State transitions
            if (d.state == State::DEFINED && blockTime >= d.startTime) {
                d.state = State::STARTED;
                d.sinceBlock = height;
                std::cout << "🚀 [VERSION BITS] '" << kv.first << "' entrou em STARTED no bloco #" << height << std::endl;
            }
            if (d.state == State::STARTED) {
                if (blockTime >= d.timeout) {
                    d.state = State::FAILED;
                    std::cout << "❌ [VERSION BITS] '" << kv.first << "' FAILED (timeout)" << std::endl;
                    continue;
                }
                if (blockVersion & (1u << (int)d.bit)) d.signalingCount++;
                // Check at end of period
                if ((height - d.sinceBlock) % d.period == 0 && height > d.sinceBlock) {
                    if (d.signalingCount >= d.threshold) {
                        d.state = State::LOCKED_IN;
                        std::cout << "🔒 [VERSION BITS] '" << kv.first << "' LOCKED_IN no bloco #" << height << std::endl;
                    }
                    d.signalingCount = 0;
                }
            }
            if (d.state == State::LOCKED_IN &&
                height >= d.sinceBlock + d.period) {
                d.state = State::ACTIVE;
                std::cout << "✅ [VERSION BITS] '" << kv.first << "' ACTIVE no bloco #" << height << std::endl;
            }
        }
    }

    inline bool isActive(const std::string& name) {
        auto it = g_deployments.find(name);
        return (it != g_deployments.end()) && (it->second.state == State::ACTIVE);
    }

    inline std::string stateName(State s) {
        switch(s) {
            case State::DEFINED:   return "DEFINED";
            case State::STARTED:   return "STARTED";
            case State::LOCKED_IN: return "LOCKED_IN";
            case State::ACTIVE:    return "ACTIVE";
            case State::FAILED:    return "FAILED";
        }
        return "UNKNOWN";
    }

    inline std::string toJSON() {
        std::string out = "[";
        bool first = true;
        for (const auto& kv : g_deployments) {
            if (!first) out += ","; first = false;
            out += "{\"name\":\"" + kv.first + "\",";
            out += "\"bit\":" + std::to_string((int)kv.second.bit) + ",";
            out += "\"state\":\"" + stateName(kv.second.state) + "\",";
            out += "\"signaling\":" + std::to_string(kv.second.signalingCount) + "}";
        }
        return out + "]";
    }

} // namespace VersionBits

#endif
