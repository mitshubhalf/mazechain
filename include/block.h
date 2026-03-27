#pragma once
#include <vector>
#include <string>
#include "transaction.h"

struct Block {
    int index;
    std::string prevHash;
    std::string hash;
    int nonce;
    std::vector<Transaction> transactions;

    Block(int idx, std::vector<Transaction> txs, std::string prev);

    std::string calculateHash();
    void mineBlock(int difficulty);
};
