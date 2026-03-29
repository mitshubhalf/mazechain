#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <string>
#include "transaction.h"

struct Block {
    int index;
    std::string data;
    std::string prevHash;
    std::string hash;
};

class Blockchain {
public:
    std::vector<Block> chain;
    std::vector<Transaction> mempool;

    Blockchain();

    void mineBlock(const std::string& miner);
    int getBalance(const std::string& address);

    void addTransaction(const Transaction& tx);
    bool validateTransaction(const Transaction& tx);

    void save();
    void load();
    bool isValid();

    // 🔥 NOVO (mempool persistente)
    void saveMempool();
    void loadMempool();

private:
    std::string sha256(const std::string& input);
};

#endif
