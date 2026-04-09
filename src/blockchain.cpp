#include "../include/blockchain.h"
#include "../include/storage.h"
#include <iostream>

Blockchain::Blockchain() {
    difficulty = 5;
    totalSupply = 0;
}

Block Blockchain::getLastBlock() {
    if (chain.empty()) return Block(-1, "0", {});
    return chain.back();
}

double Blockchain::getBlockReward(int height) {
    double reward = 250.0;
    int halvings = height / 1000;
    for (int i = 0; i < halvings; i++) reward /= 2.0;

    if (totalSupply >= 20000000) return 0; // 🔥 Limite de 20M
    return reward;
}

void Blockchain::mineBlock(std::string minerAddress) {
    if (chain.empty()) {
        Block genesis(0, "0", {});
        genesis.mine(difficulty);
        chain.push_back(genesis);
    }

    double reward = getBlockReward(chain.size());
    Transaction coinbase({}, { {minerAddress, reward} });
    Block newBlock(chain.size(), getLastBlock().hash, {coinbase});

    std::cout << "⛏️ Mining block " << newBlock.index << "...\n";
    newBlock.mine(difficulty);

    chain.push_back(newBlock);
    totalSupply += reward;

    Storage::saveChain(*this, "data/blockchain.dat");
    std::cout << "🎉 Reward: " << reward << "\n";
}

double Blockchain::getBalance(std::string address) {
    double balance = 0;
    for (const auto &block : chain)
        for (const auto &tx : block.transactions)
            for (const auto &out : tx.vout)
                if (out.address == address) balance += out.amount;
    return balance;
}

void Blockchain::send(std::string from, std::string to, double amount) {
    if (getBalance(from) < amount) {
        std::cout << "❌ Saldo insuficiente\n";
        return;
    }
    Transaction tx({}, { {to, amount}, {from, getBalance(from) - amount} });
    Block newBlock(chain.size(), getLastBlock().hash, {tx});
    newBlock.mine(difficulty);
    chain.push_back(newBlock);
    Storage::saveChain(*this, "data/blockchain.dat");
}

std::vector<Block> Blockchain::getChain() const { return chain; }
int Blockchain::getDifficulty() const { return difficulty; }

void Blockchain::clearChain() { 
    chain.clear(); 
    totalSupply = 0; 
}

void Blockchain::addBlock(const Block& block) {
    chain.push_back(block);
    for(const auto& tx : block.transactions)
        for(const auto& out : tx.vout) totalSupply += out.amount;
}
