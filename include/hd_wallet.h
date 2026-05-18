#ifndef HD_WALLET_H
#define HD_WALLET_H

// ── MazeChain HD Wallet — BIP-32 Hierárquico Determinístico ──────────────────
// Uma seed gera infinitas carteiras filhas com caminhos de derivação.
// Padrão: m/44'/1611'/account'/change/index
// 1611 = coin type MazeChain (derivado do ano 2026 - 415 = 1611, número de fantasia)

#include <string>
#include <vector>
#include <cstdint>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include "crypto.h"

namespace HDWallet {

    // Chave mestre derivada da seed
    struct HDKey {
        std::string privateKey;   // 32 bytes hex
        std::string chainCode;    // 32 bytes hex
        std::string publicKey;    // chave pública comprimida (hex)
        std::string address;      // endereço MazeChain (MZ...)
        uint32_t    index;        // índice de derivação
        std::string path;         // ex: "m/44'/1611'/0'/0/3"
    };

    // Converte bytes para hex string
    inline std::string bytes_to_hex(const uint8_t* data, size_t len) {
        const char* hex = "0123456789abcdef";
        std::string out;
        out.reserve(len * 2);
        for (size_t i = 0; i < len; i++) {
            out += hex[(data[i] >> 4) & 0xF];
            out += hex[data[i] & 0xF];
        }
        return out;
    }

    // Converte hex string para bytes
    inline std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
        std::vector<uint8_t> bytes;
        bytes.reserve(hex.size() / 2);
        for (size_t i = 0; i + 1 < hex.size(); i += 2) {
            uint8_t byte = (uint8_t)std::stoul(hex.substr(i, 2), nullptr, 16);
            bytes.push_back(byte);
        }
        return bytes;
    }

    // HMAC-SHA512(key, data) → 64 bytes
    inline std::vector<uint8_t> hmac_sha512(
        const uint8_t* key, size_t key_len,
        const uint8_t* data, size_t data_len)
    {
        std::vector<uint8_t> result(64);
        unsigned int out_len = 64;
        HMAC(EVP_sha512(), key, (int)key_len, data, data_len, result.data(), &out_len);
        return result;
    }

    // Gera chave mestre a partir da seed phrase (BIP-32)
    // Usa HMAC-SHA512("MazeChain seed", seed_bytes) em vez de "Bitcoin seed"
    inline HDKey masterKey(const std::string& seed_phrase) {
        const std::string hmac_key = "MazeChain seed";
        std::vector<uint8_t> data(seed_phrase.begin(), seed_phrase.end());

        auto result = hmac_sha512(
            (const uint8_t*)hmac_key.data(), hmac_key.size(),
            data.data(), data.size()
        );

        HDKey key;
        key.privateKey = bytes_to_hex(result.data(), 32);      // IL
        key.chainCode  = bytes_to_hex(result.data() + 32, 32); // IR
        key.index      = 0;
        key.path       = "m";

        // Deriva endereço MZ a partir da chave privada
        std::string h1 = Crypto::sha256_util(key.privateKey);
        std::string h2 = Crypto::sha256_util(h1 + "SALT_MAZE_2026_PRODUCTION");
        key.address    = "MZ" + h2.substr(0, 32);
        key.publicKey  = h1;

        return key;
    }

    // Deriva uma chave filha a partir de uma chave pai + índice (BIP-32 normal derivation)
    inline HDKey deriveChild(const HDKey& parent, uint32_t index) {
        // Data = parent_publicKey || index (big-endian)
        std::vector<uint8_t> data;
        data.reserve(36);
        auto pub_bytes = hex_to_bytes(parent.publicKey);
        for (auto b : pub_bytes) data.push_back(b);
        data.push_back((index >> 24) & 0xFF);
        data.push_back((index >> 16) & 0xFF);
        data.push_back((index >>  8) & 0xFF);
        data.push_back((index >>  0) & 0xFF);

        auto chain_bytes = hex_to_bytes(parent.chainCode);

        auto result = hmac_sha512(
            chain_bytes.data(), chain_bytes.size(),
            data.data(), data.size()
        );

        // child_key = (parent_key + IL) mod n (simplificado: xor para MazeChain)
        auto parent_bytes = hex_to_bytes(parent.privateKey);
        std::vector<uint8_t> child_key(32);
        for (int i = 0; i < 32; i++) {
            child_key[i] = parent_bytes[i] ^ result[i]; // XOR determinístico
        }

        HDKey child;
        child.privateKey = bytes_to_hex(child_key.data(), 32);
        child.chainCode  = bytes_to_hex(result.data() + 32, 32);
        child.index      = index;
        child.path       = parent.path + "/" + std::to_string(index);

        // Deriva endereço MZ filho
        std::string h1 = Crypto::sha256_util(child.privateKey + "MAZE_HD_CHILD");
        std::string h2 = Crypto::sha256_util(h1 + "SALT_MAZE_2026_PRODUCTION");
        child.address   = "MZ" + h2.substr(0, 32);
        child.publicKey = h1;

        return child;
    }

    // Atalho: deriva endereço no caminho m/44'/1611'/0'/0/index
    // account=0, change=0, index=qualquer
    inline HDKey deriveAddress(const std::string& seed_phrase, uint32_t index) {
        HDKey master = masterKey(seed_phrase);
        // Deriva 4 níveis: account(0), purpose(44), coin(1611), change(0)
        HDKey k1 = deriveChild(master, 44);
        HDKey k2 = deriveChild(k1, 1611);
        HDKey k3 = deriveChild(k2, 0);
        HDKey k4 = deriveChild(k3, 0);
        HDKey k5 = deriveChild(k4, index);
        k5.path  = "m/44'/1611'/0'/0/" + std::to_string(index);
        return k5;
    }

    // Gera lista de endereços (gap limit padrão = 20)
    inline std::vector<HDKey> generateAddresses(const std::string& seed_phrase, int count = 20) {
        std::vector<HDKey> addrs;
        addrs.reserve(count);
        for (int i = 0; i < count; i++) {
            addrs.push_back(deriveAddress(seed_phrase, (uint32_t)i));
        }
        return addrs;
    }

} // namespace HDWallet

#endif
