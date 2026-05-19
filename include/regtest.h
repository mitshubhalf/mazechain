#ifndef REGTEST_H
#define REGTEST_H

// MazeChain — Regtest & Devnet Modes
// Regtest: instant mining, difficulty=1, isolated, for development/testing
// Devnet:  custom genesis, configurable parameters for QA networks
// Both use different address prefixes and magic bytes

#include <string>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <functional>
#include <iostream>

namespace Regtest {

    enum class Mode { MAINNET, TESTNET, REGTEST, DEVNET };

    inline Mode currentMode() {
        if (std::getenv("MAZECHAIN_REGTEST") &&
            std::string(std::getenv("MAZECHAIN_REGTEST")) == "1") return Mode::REGTEST;
        if (std::getenv("MAZECHAIN_DEVNET") &&
            std::string(std::getenv("MAZECHAIN_DEVNET")) == "1") return Mode::DEVNET;
        if (std::getenv("MAZECHAIN_TESTNET") &&
            std::string(std::getenv("MAZECHAIN_TESTNET")) == "1") return Mode::TESTNET;
        return Mode::MAINNET;
    }

    inline bool isRegtest() { return currentMode() == Mode::REGTEST; }
    inline bool isTestnet() { return currentMode() == Mode::TESTNET; }
    inline bool isDevnet()  { return currentMode() == Mode::DEVNET; }
    inline bool isMainnet() { return currentMode() == Mode::MAINNET; }

    struct NetworkParams {
        std::string name;
        std::string addressPrefix;
        std::string magic;          // 4-byte network magic
        std::string genesisMsg;
        long        genesisTimestamp;
        int         minDifficulty;
        int         halvingInterval;// blocks between halvings
        double      initialReward;
        std::string rpcPort;
        std::string p2pPort;
        bool        instantMine;    // regtest: mine instantly
        bool        allowPrivatePeers;
        std::string dataSubdir;
    };

    inline NetworkParams getParams() {
        Mode m = currentMode();
        switch (m) {
            case Mode::REGTEST:
                return {"regtest", "RZ", "fabfb5da",
                        "MAZE_REGTEST_GENESIS: Development block",
                        1000000000L, 1, 150, 50.0, "20003", "20004",
                        true, true, "regtest"};

            case Mode::TESTNET:
                return {"testnet", "TZ", "0b110907",
                        "MAZE_TESTNET_GENESIS: MazeChain testnet - use freely!",
                        1714158300L, 1, 10000, 100.0, "10003", "10004",
                        false, true, "testnet"};

            case Mode::DEVNET: {
                const char* devGenesis = std::getenv("DEVNET_GENESIS_MSG");
                return {"devnet", "DZ", "aabbccdd",
                        devGenesis ? devGenesis : "MAZE_DEVNET_GENESIS: Custom dev network",
                        1700000000L, 1, 100, 100.0, "30003", "30004",
                        false, true, "devnet"};
            }

            default: // MAINNET
                return {"mainnet", "MZ", "f9beb4d9",
                        "MAZE_GENESIS_MESSAGE: The Times 21/Apr/2026 MazeChain reborn.",
                        1714158289L, 4, 10000, 100.0, "10002", "10001",
                        false, false, "."};
        }
    }

    inline std::string dataDir() {
        auto p = getParams();
        return "data" + (p.dataSubdir == "." ? "" : "/" + p.dataSubdir);
    }

    // Regtest: generate blocks instantly (difficulty 1, no PoW grind)
    inline std::string regtestMineBlock(int index, const std::string& prevHash) {
        if (!isRegtest()) return "";
        // Ultra-fast hash: just needs one leading zero (difficulty=1)
        return "0" + std::to_string(index) + prevHash.substr(0, 30) + "regtest";
    }

    // Faucet: regtest/testnet only — send coins to address
    inline bool canUseFaucet() { return !isMainnet(); }

    // Network upgrade signaling
    struct NetworkUpgrade {
        std::string name;
        int         activationHeight;
        bool        active;
    };

    inline std::vector<NetworkUpgrade> getUpgrades(int currentHeight) {
        std::vector<NetworkUpgrade> upgrades = {
            {"SegWit",      0,     true},
            {"Taproot",     0,     true},
            {"CSV/CLTV",    0,     true},
            {"Schnorr",     0,     true},
            {"Covenants", 500000,  currentHeight >= 500000},
            {"Vaults",    500000,  currentHeight >= 500000},
        };
        if (isRegtest() || isTestnet()) {
            for (auto& u : upgrades) { u.activationHeight = 0; u.active = true; }
        }
        return upgrades;
    }

    // Emergency checkpoint mechanism
    struct EmergencyCheckpoint {
        int         height;
        std::string hash;
        int64_t     timestamp;
        std::string signingKey; // authority signing key (for verification)
    };

    static std::vector<EmergencyCheckpoint> g_emergencyCheckpoints;

    inline void addEmergencyCheckpoint(int height, const std::string& hash,
                                        const std::string& sigKey = "") {
        g_emergencyCheckpoints.push_back({height, hash, (int64_t)std::time(nullptr), sigKey});
        std::cout << "🚨 [EMERGENCY CHECKPOINT] Bloco #" << height << " registrado como ponto emergencial." << std::endl;
    }

    inline bool verifyAgainstEmergency(int height, const std::string& hash) {
        for (const auto& cp : g_emergencyCheckpoints) {
            if (cp.height == height) return cp.hash == hash;
        }
        return true; // no emergency checkpoint for this height
    }

    // Hard fork handling
    inline bool isHardForkBlock(int height) {
        // Define hard fork activation heights
        static const std::vector<int> hardForks = {}; // add when needed
        for (int h : hardForks) if (h == height) return true;
        return false;
    }

    // Soft fork mechanism: return true if feature is active
    inline bool isSoftForkActive(const std::string& feature, int height) {
        if (isRegtest() || isTestnet()) return true;
        // Mainnet activation heights
        static const std::map<std::string,int> activations = {
            {"segwit", 0}, {"taproot", 0}, {"schnorr", 0},
            {"csv", 0}, {"cltv", 0}, {"rbf", 0}
        };
        auto it = activations.find(feature);
        return (it != activations.end()) && (height >= it->second);
    }

} // namespace Regtest

#endif
