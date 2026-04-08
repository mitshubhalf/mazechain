#ifndef BLOCK_H
#define BLOCK_H

#include <string>
#include <vector>
#include "transaction.h"

class Block {
public:
    // 🔥 CONSTRUTOR VAZIO (ESSENCIAL PRO LOAD)
    Block() {}

    int index;
    std::string timestamp;
    std::vector<Transaction> transactions;
    std::string previousHash;
    std::string hash;
    int nonce;

    // 🔥 CONSTRUTOR PRINCIPAL
    Block(int idx, std::vector<Transaction> txs, std::string prevHash);

    std::string calculateHash() const;
    void mineBlock(int difficulty);
};

#endif
