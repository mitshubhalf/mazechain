#include "../include/blockchain.h"

Blockchain::Blockchain() {
    chain.push_back(Block(0, {}, "0"));
}

Block Blockchain::getLatestBlock() const {
    return chain.back();
}

void Blockchain::minePendingTransactions(std::string minerAddress) {

    // recompensa do minerador
    pendingTransactions.push_back(Transaction("SYSTEM", minerAddress, 50));

    Block newBlock(chain.size(), pendingTransactions, getLatestBlock().hash);

    newBlock.mineBlock(4);

    chain.push_back(newBlock);

    pendingTransactions.clear();
}

void Blockchain::addTransaction(const Transaction& tx) {
    pendingTransactions.push_back(tx);
}

int Blockchain::getBalance(std::string address) {

    int balance = 0;

    for (const auto& block : chain) {
        for (const auto& tx : block.transactions) {

            if (tx.from == address)
                balance -= tx.amount;

            if (tx.to == address)
                balance += tx.amount;
        }
    }

    return balance;
}

const std::vector<Block>& Blockchain::getChain() const {
    return chain;
}

void Blockchain::addLoadedBlock(const Block& block) {
    chain.push_back(block);
}

void Blockchain::clearChain() {
    chain.clear();
}
