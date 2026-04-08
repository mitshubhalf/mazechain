#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include "block.h"
#include "transaction.h"

class Blockchain {
private:
    std::vector<Block> chain;
    std::vector<Transaction> pendingTransactions;

public:
    Blockchain();

    void addBlock(Block newBlock);
    Block getLatestBlock() const;

    void clearChain();
    void addLoadedBlock(const Block& block);

    std::vector<Block>& getChain();
    const std::vector<Block>& getChain() const;

    void addTransaction(const Transaction& tx);
    void minePendingTransactions(const std::string& minerAddress);

    double getBalance(const std::string& address) const;

    // 🔥 NOVO
    bool isChainValid() const;
};

#endif
