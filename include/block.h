#ifndef BLOCK_H
#define BLOCK_H

#include <vector>
#include <string>
#include "transaction.h"

class Block {
public:
    int index;
    long long timestamp;
    std::string hash;
    std::string prevHash;
    int nonce;
    std::vector<Transaction> transactions;

    Block(int idx, std::string prev, std::vector<Transaction> txs);
    std::string calculateHash() const;
    void mine(int difficulty); // A lógica bruta vai para o miner.cpp
};

#endif
