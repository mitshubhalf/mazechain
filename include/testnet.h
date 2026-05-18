#ifndef TESTNET_H
#define TESTNET_H

// ── MazeChain — Configuração de Rede ─────────────────────────────────────────
// Mainnet: endereços MZ..., genesis timestamp 1714158289
// Testnet: endereços TZ..., genesis diferente, dificuldade mínima = 1
//
// Ativação: variável de ambiente MAZECHAIN_TESTNET=1

#include <string>
#include <cstdlib>

namespace NetworkConfig {

    // Lê a rede ativa na inicialização
    inline bool isTestnet() {
        const char* env = std::getenv("MAZECHAIN_TESTNET");
        return (env != nullptr && std::string(env) == "1");
    }

    struct Config {
        std::string name;           // "mainnet" ou "testnet"
        std::string address_prefix; // "MZ" ou "TZ"
        std::string genesis_message;
        long        genesis_timestamp;
        int         min_difficulty;
        std::string magic_bytes;    // 4 bytes identificadores da rede (hex)
        std::string protocol_version;
    };

    inline Config getConfig() {
        if (isTestnet()) {
            return Config{
                "testnet",
                "TZ",
                "MAZE_TESTNET_GENESIS: MazeChain testnet - use freely!",
                1714158300, // timestamp diferente do mainnet
                1,          // dificuldade mínima 1 para testes rápidos
                "0b110907", // magic bytes do testnet
                "4.0.0-testnet"
            };
        }
        return Config{
            "mainnet",
            "MZ",
            "MAZE_GENESIS_MESSAGE: The Times 21/Apr/2026 MazeChain reborn.",
            1714158289,
            4,          // dificuldade mínima Era Genesis
            "f9beb4d9", // magic bytes do mainnet
            "4.0.0"
        };
    }

    // Valida se um endereço pertence à rede atual
    inline bool isValidAddress(const std::string& addr) {
        auto cfg = getConfig();
        if (addr.size() < 30) return false;
        return addr.substr(0, cfg.address_prefix.size()) == cfg.address_prefix
            || addr.substr(0, 4) == "MZms" // multisig sempre válido
            || addr == "MZ_GENESIS_ADDRESS"
            || addr == "MZ_SYSTEM_RESERVE_FUND_NON_EXPENDABLE";
    }

} // namespace NetworkConfig

#endif
