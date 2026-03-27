#pragma once
#include <string>
#include <vector>
#include "transaction.h"

struct Block {
    int index;
    std::string prevHash;
    std::vector<Transaction> transactions;
    std::string hash;
    int nonce;

    Block(int i, std::vector<Transaction> txs, std::string prev);

    std::string calculateHash();
    void mineBlock(int difficulty);
};
