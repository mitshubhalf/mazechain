#include "../include/crypto.h"
#include "../include/transaction.h"
#include <openssl/sha.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/ecdsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/evp.h> // Adicionado para AES/EVP
#include <openssl/aes.h> // ADICIONADO PARA DEFINIÇÃO DE AES_BLOCK_SIZE
#include <iomanip>
#include <sstream>
#include <vector>
#include <iostream>

namespace Crypto {

// Helper para converter bytes em Hexadecimal
std::string to_hex(const unsigned char* data, size_t len) {
    std::stringstream ss;
    for (size_t i = 0; i < len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    }
    return ss.str();
}

// Helper para converter Hexadecimal em bytes
std::vector<unsigned char> from_hex(const std::string& hex) {
    std::vector<unsigned char> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        bytes.push_back((unsigned char)std::stoi(hex.substr(i, 2), nullptr, 16));
    }
    return bytes;
}

// --- SHA256 SIMPLES ---
std::string sha256_util(std::string str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)str.c_str(), str.size(), hash);
    return to_hex(hash, SHA256_DIGEST_LENGTH);
}

// --- SHA256 DUPLO (PADRÃO BITCOIN) ---
std::string sha256d(std::string str) {
    unsigned char hash1[SHA256_DIGEST_LENGTH];
    unsigned char hash2[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)str.c_str(), str.size(), hash1);
    SHA256(hash1, SHA256_DIGEST_LENGTH, hash2);
    return to_hex(hash2, SHA256_DIGEST_LENGTH);
}

// --- Merkle Root (BITCOIN STYLE) ---
std::string calculateMerkleRoot(const std::vector<Transaction>& txs) {
    if (txs.empty()) return sha256d("empty_block");
    std::vector<std::string> tree;
    for (const auto& tx : txs) tree.push_back(tx.id);

    while (tree.size() > 1) {
        if (tree.size() % 2 != 0) tree.push_back(tree.back());
        std::vector<std::string> nextLevel;
        for (size_t i = 0; i < tree.size(); i += 2) {
            nextLevel.push_back(sha256d(tree[i] + tree[i+1]));
        }
        tree = nextLevel;
    }
    return tree[0];
}

// --- GERAÇÃO DE CHAVES ECDSA (SECP256K1) ---
KeyPair generate_keys_from_seed(const std::string& seed) {
    KeyPair kp;

    // 1. Gerar Chave Privada a partir da Seed (Determinística)
    std::string hashed_seed = sha256d(seed);
    kp.private_key = hashed_seed;

    // 2. Usar OpenSSL para derivar a Chave Pública usando a curva secp256k1
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
    BIGNUM* priv_bn = BN_new();
    BN_hex2bn(&priv_bn, hashed_seed.c_str());

    EC_GROUP* group = (EC_GROUP*)EC_KEY_get0_group(key);
    EC_POINT* pub_point = EC_POINT_new(group);

    // pub_key = group_G * priv_key
    EC_POINT_mul(group, pub_point, priv_bn, NULL, NULL, NULL);
    EC_KEY_set_private_key(key, priv_bn);
    EC_KEY_set_public_key(key, pub_point);

    // Converter ponto da chave pública para Hex (Formato Comprimido)
    char* pub_hex = EC_POINT_point2hex(group, pub_point, POINT_CONVERSION_COMPRESSED, NULL);
    kp.public_key = std::string(pub_hex);

    // Limpeza
    OPENSSL_free(pub_hex);
    EC_POINT_free(pub_point);
    BN_free(priv_bn);
    EC_KEY_free(key);

    return kp;
}

// --- ASSINATURA DIGITAL COMPLETA ---
std::string sign_data(const std::string& data, const std::string& private_key_hex) {
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
    BIGNUM* priv_bn = BN_new();
    BN_hex2bn(&priv_bn, private_key_hex.c_str());
    EC_KEY_set_private_key(key, priv_bn);

    // O dado a ser assinado deve ser o Hash do conteúdo
    std::string message_hash_str = sha256d(data);
    std::vector<unsigned char> message_hash = from_hex(message_hash_str);

    unsigned char sig_buf[256];
    unsigned int sig_len;

    if (ECDSA_sign(0, message_hash.data(), message_hash.size(), sig_buf, &sig_len, key) == 0) {
        EC_KEY_free(key);
        BN_free(priv_bn);
        return "ERROR_SIGNING";
    }

    std::string signature = to_hex(sig_buf, sig_len);

    EC_KEY_free(key);
    BN_free(priv_bn);
    return signature;
}

// --- VERIFICAÇÃO RIGOROSA ---
bool verify_signature(const std::string& data, const std::string& signature_hex, const std::string& public_key_hex) {
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
    EC_GROUP* group = (EC_GROUP*)EC_KEY_get0_group(key);

    // Carregar a chave pública a partir do Hex
    EC_POINT* pub_point = EC_POINT_new(group);
    EC_POINT_hex2point(group, public_key_hex.c_str(), pub_point, NULL);
    EC_KEY_set_public_key(key, pub_point);

    // Hash do dado original
    std::string message_hash_str = sha256d(data);
    std::vector<unsigned char> message_hash = from_hex(message_hash_str);

    // Converter assinatura de Hex para bytes
    std::vector<unsigned char> sig_bytes = from_hex(signature_hex);

    // Verificação matemática
    int result = ECDSA_verify(0, message_hash.data(), message_hash.size(), sig_bytes.data(), sig_bytes.size(), key);

    EC_POINT_free(pub_point);
    EC_KEY_free(key);

    return (result == 1);
}

// --- MELHORIA: CRIPTOGRAFIA DE WALLET (AES-256-CBC) ---

std::string encrypt_data(const std::string& plaintext, const std::string& password) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    unsigned char key[32], iv[16];

    // Deriva chave e IV da senha de forma simples para a MazeChain
    EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha256(), NULL, (unsigned char*)password.c_str(), password.length(), 1, key, iv);

    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);

    std::vector<unsigned char> ciphertext(plaintext.length() + AES_BLOCK_SIZE);
    int len, ciphertext_len;

    EVP_EncryptUpdate(ctx, ciphertext.data(), &len, (unsigned char*)plaintext.c_str(), plaintext.length());
    ciphertext_len = len;

    EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len);
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return to_hex(ciphertext.data(), ciphertext_len);
}

std::string decrypt_data(const std::string& ciphertext_hex, const std::string& password) {
    std::vector<unsigned char> ciphertext = from_hex(ciphertext_hex);
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    unsigned char key[32], iv[16];

    EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha256(), NULL, (unsigned char*)password.c_str(), password.length(), 1, key, iv);

    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);

    std::vector<unsigned char> plaintext(ciphertext.size());
    int len, plaintext_len;

    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ciphertext.size()) <= 0) return "ERROR";
    plaintext_len = len;

    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) <= 0) {
        EVP_CIPHER_CTX_free(ctx);
        return "DECRYPT_FAIL"; // Senha incorreta
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return std::string((char*)plaintext.data(), plaintext_len);
}

} // namespace Crypto