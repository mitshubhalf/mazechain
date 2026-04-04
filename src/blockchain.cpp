#include "../include/blockchain.h"

Blockchain::Blockchain() {
    chain.push_back(Block(0, {}, "0"));
}

void Blockchain::addBlock(Block newBlock) {
    chain.push_back(newBlock);
}

Block Blockchain::getLatestBlock() const {
    return chain.back();
}

void Blockchain::clearChain() {
    chain.clear();
}

void Blockchain::addLoadedBlock(const Block& block) {
    chain.push_back(block);
}

std::vector<Block>& Blockchain::getChain() {
    return chain;
}

void Blockchain::addTransaction(const Transaction& tx) {
    if (!chain.empty()) {
        chain.back().transactions.push_back(tx);
    }
}

double Blockchain::getBalance(const std::string& address) {
    double balance = 0;

    for (const auto& block : chain) {
        for (const auto& tx : block.transactions) {
            if (tx.from == address) balance -= tx.amount;
            if (tx.to == address) balance += tx.amount;
        }
    }

    return balance;
}
