#ifndef BLOCK_H
#define BLOCK_H

#include <vector>
#include <string>
#include "transaction.h"

class Block {
public:
    int index;
    std::string previousHash;
    std::string hash;
    std::vector<Transaction> transactions;
    int nonce;

    Block(int idx, std::vector<Transaction> txs, std::string prevHash);

    std::string calculateHash() const;
    void mineBlock(int difficulty);
};

#endif
