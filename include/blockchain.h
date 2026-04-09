#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include "block.h"

struct UTXO {
    std::string txId;
    int index;
    std::string address;
    double amount;
};

class Blockchain {
private:
    std::vector<Block> chain;
    std::vector<Transaction> pendingTransactions;
    std::vector<UTXO> utxoPool;

public:
    Blockchain();

    Block getLatestBlock() const;
    const std::vector<Block>& getChain() const;

    void addBlock(Block block);
    void addTransaction(const Transaction& tx);
    void minePendingTransactions(const std::string& minerAddress);

    double getBalance(const std::string& address) const;

    void rebuildUTXO();

    // 🔥 ESSENCIAL PRO STORAGE
    void replaceChain(const std::vector<Block>& newChain);
};

#endif
