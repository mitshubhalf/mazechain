#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include "../include/block.h"
#include "../include/transaction.h"

class Blockchain {
private:
    std::vector<Block> chain;
    std::vector<Transaction> pendingTransactions;

public:
    Blockchain();

    Block getLatestBlock() const;

    void minePendingTransactions(std::string minerAddress);

    void addTransaction(const Transaction& tx);

    int getBalance(std::string address);

    const std::vector<Block>& getChain() const;

    void addLoadedBlock(const Block& block);

    void clearChain();
};

#endif
