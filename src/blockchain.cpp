#include "../include/blockchain.h"
#include "../include/storage.h"
#include <iostream>

Blockchain::Blockchain() {
    difficulty = 5;
    totalSupply = 0;
    chain.push_back(Block(0, "0", {}));
}

Block Blockchain::getLastBlock() {
    return chain.back();
}

double Blockchain::getBlockReward(int height) {
    double reward = 250;
    int halvings = height / 1000;

    for (int i = 0; i < halvings; i++)
        reward /= 2;

    if (totalSupply >= 20000000)
        return 0;

    return reward;
}

void Blockchain::mineBlock(std::string minerAddress) {
    double reward = getBlockReward(chain.size());

    Transaction coinbase({}, { {minerAddress, reward} });

    Block newBlock(chain.size(), getLastBlock().hash, {coinbase});

    std::cout << "⛏️ Mining...\n";

    newBlock.mine(difficulty);

    std::string target(difficulty, '0');

    if (newBlock.hash != newBlock.calculateHash()) {
        std::cout << "❌ Hash inválido!\n";
        return;
    }

    if (newBlock.hash.substr(0, difficulty) != target) {
        std::cout << "❌ PoW inválido!\n";
        return;
    }

    if (newBlock.prevHash != getLastBlock().hash) {
        std::cout << "❌ Fork detectado!\n";
        return;
    }

    chain.push_back(newBlock);
    totalSupply += reward;

    std::cout << "🎉 Reward: " << reward << "\n";

    Storage::saveChain(*this, "data/blockchain.dat");
}

double Blockchain::getBalance(std::string address) {
    double balance = 0;

    for (auto &block : chain)
        for (auto &tx : block.transactions)
            for (auto &out : tx.vout)
                if (out.address == address)
                    balance += out.amount;

    return balance;
}

void Blockchain::send(std::string from, std::string to, double amount) {
    if (getBalance(from) < amount) {
        std::cout << "❌ Saldo insuficiente\n";
        return;
    }

    Transaction tx({}, { {to, amount} });

    Block newBlock(chain.size(), getLastBlock().hash, {tx});

    std::cout << "⛏️ Mining transaction...\n";

    newBlock.mine(difficulty);

    std::string target(difficulty, '0');

    if (newBlock.hash != newBlock.calculateHash()) {
        std::cout << "❌ Hash inválido!\n";
        return;
    }

    if (newBlock.hash.substr(0, difficulty) != target) {
        std::cout << "❌ PoW inválido!\n";
        return;
    }

    if (newBlock.prevHash != getLastBlock().hash) {
        std::cout << "❌ Fork detectado!\n";
        return;
    }

    chain.push_back(newBlock);

    Storage::saveChain(*this, "data/blockchain.dat");

    std::cout << "✅ Transação confirmada\n";
}

/* =========================
   FUNÇÕES OBRIGATÓRIAS
   ========================= */

std::vector<Block> Blockchain::getChain() const {
    return chain;
}

void Blockchain::clearChain() {
    chain.clear();
}

int Blockchain::getDifficulty() const {
    return difficulty;
}

void Blockchain::addBlock(const Block& block) {
    chain.push_back(block);
}
