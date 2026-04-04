#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include "block.h"
#include "transaction.h"

class Blockchain {
private:
    std::vector<Block> chain;
    int difficulty;

public:
    std::vector<Transaction> pendingTransactions;

    Blockchain();

    void addBlock(Block newBlock);
    Block getLatestBlock() const;

    void minePendingTransactions(std::string minerAddress);

    int getBalance(const std::string& address) const;
    bool addTransaction(const Transaction& tx);

    void printChain() const;

    const std::vector<Block>& getChain() const;
    void clearChain();
    void addLoadedBlock(const Block& b);
};

#endif
