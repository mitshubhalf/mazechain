#pragma once
#include <vector>
#include <string>
#include "transaction.h"

struct Block {
    int index;
    std::vector<Transaction> transactions;
    std::string prevHash;
    std::string hash;
    int nonce;

    Block(int idx, std::vector<Transaction> txs, std::string prev);

    std::string calculateHash();
    void mineBlock(int difficulty);
};
