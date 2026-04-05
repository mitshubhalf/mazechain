#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <string>
#include "block.h"
#include "transaction.h"

class Blockchain {
private:
    std::vector<Block> chain;

    // 🔥 FALTAVA ISSO (ERRO PRINCIPAL)
    std::vector<Transaction> pendingTransactions;

public:
    Blockchain();

    void addBlock(Block newBlock);
    Block getLatestBlock() const;

    void clearChain();
    void addLoadedBlock(const Block& block);

    const std::vector<Block>& getChain() const;

    void addTransaction(const Transaction& tx);
    double getBalance(const std::string& address);

    void minePendingTransactions(const std::string& minerAddress);
};

#endif
