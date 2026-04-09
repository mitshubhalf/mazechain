#ifndef BLOCK_H
#define BLOCK_H

#include <vector>
#include <string>
#include "transaction.h"

class Block {
public:
    int index;
    std::string prevHash;
    std::string hash;
    long nonce;
    std::vector<Transaction> transactions;

    Block(int idx, std::string prev, std::vector<Transaction> txs);

    std::string calculateHash() const;
    void mine(int difficulty);
};

#endif
