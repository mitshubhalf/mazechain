#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <string>
#include "block.h"
#include "transaction.h"

class Blockchain {
private:
    std::vector<Block> chain;
    std::vector<Transaction> pendingTransactions;

public:
    Blockchain();

    // blocos
    void addBlock(Block newBlock);
    Block getLatestBlock() const;

    // persistência
    void clearChain();
    void addLoadedBlock(const Block& block);

    // acesso
    std::vector<Block>& getChain();
    const std::vector<Block>& getChain() const;

    // transações
    void addTransaction(const Transaction& tx);
    void minePendingTransactions(const std::string& minerAddress);

    // saldo
    double getBalance(const std::string& address) const; // 🔥 CONST ADICIONADO
};

#endif
