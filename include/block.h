#pragma once
#include <string>
#include <vector>

struct Transaction {
    std::string from;
    std::string to;
    double amount;
    std::string signature;
};

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
