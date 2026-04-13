#include "../include/block.h"
#include "../include/crypto.h"
#include <iostream>
#include <sstream>
#include <ctime>

Block::Block(int idx, std::string prev, std::vector<Transaction> txs) {
    index = idx;
    prevHash = prev;
    transactions = txs;
    timestamp = std::time(0);
    nonce = 0;
    hash = calculateHash();
}

std::string Block::calculateHash() const {
    std::stringstream ss;
    std::string root = Crypto::calculateMerkleRoot(this->transactions);
    ss << index << (long long)timestamp << prevHash << nonce << root;
    return Crypto::sha256_util(ss.str());
}

void Block::mine(int difficulty) {
    std::string target(difficulty, '0');
    while (hash.substr(0, difficulty) != target) {
        nonce++;
        hash = calculateHash();
    }
    std::cout << "🎯 Bloco #" << index << " minerado! Hash: " << hash << std::endl;
}
