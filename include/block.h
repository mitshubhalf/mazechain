#ifndef BLOCK_H
#define BLOCK_H

#include <vector>
#include <string>
#include <ctime>
#include "transaction.h"

class Block {
public:
    int index;
    std::string prevHash;
    std::vector<Transaction> transactions;
    long timestamp;
    int nonce;
    std::string hash;

    Block(int idx, std::string prev, std::vector<Transaction> txs);

    std::string calculateHash() const;
    void mine(int difficulty);
};

#endif
