#include "../include/blockchain.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <chrono>

// Função estável de SHA256
std::string sha256_util(const std::string str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

Block::Block(int idx, std::string prev, std::vector<Transaction> txs) {
    index = idx;
    prevHash = prev;
    transactions = txs;
    auto now = std::chrono::system_clock::now();
    timestamp = std::chrono::system_clock::to_time_t(now);
    nonce = 0;
    hash = calculateHash();
}

std::string Block::calculateHash() const {
    std::stringstream ss;
    // O uso de stringstream evita alocações temporárias pesadas
    ss << index << timestamp << prevHash << nonce;
    for (const auto& tx : transactions) {
        for (const auto& out : tx.vout) {
            ss << out.address << out.amount;
        }
    }
    return sha256_util(ss.str());
}

void Block::mine(int difficulty) {
    std::string target(difficulty, '0');
    while (hash.substr(0, difficulty) != target) {
        nonce++;
        hash = calculateHash();
        if (nonce % 1000000 == 0) {
            std::cout << "Minerando... Nonce: " << nonce << " | Hash: " << hash.substr(0, 10) << "..." << std::endl;
        }
    }
    std::cout << "🎯 Bloco Minerado! Hash: " << hash << std::endl;
}
