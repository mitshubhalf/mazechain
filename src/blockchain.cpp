#include "../include/blockchain.h"
#include <iostream>

Blockchain::Blockchain() {
    difficulty = 4;
    totalSupply = 0;

    chain.push_back(Block(0, "0", {}));
}

Block Blockchain::getLastBlock() {
    return chain.back();
}

int Blockchain::adjustDifficulty() {
    if (chain.size() < 2)
        return difficulty;

    Block last = chain.back();
    Block prev = chain[chain.size() - 2];

    long timeTaken = last.timestamp - prev.timestamp;

    if (timeTaken < 5)
        difficulty++;
    else if (timeTaken > 15 && difficulty > 1)
        difficulty--;

    std::cout << "⏱ Tempo: " << timeTaken << "s | Diff: " << difficulty << "\n";

    return difficulty;
}

bool Blockchain::isValidBlock(const Block& newBlock, const Block& previousBlock) {
    // 🔗 ligação correta
    if (newBlock.prevHash != previousBlock.hash)
        return false;

    // 🔢 hash correto
    if (newBlock.calculateHash() != newBlock.hash)
        return false;

    // ⛏️ prova de trabalho
    std::string target(difficulty, '0');
    if (newBlock.hash.substr(0, difficulty) != target)
        return false;

    return true;
}

void Blockchain::addBlock(const Block& block) {
    if (!chain.empty()) {
        if (!isValidBlock(block, getLastBlock())) {
            std::cout << "❌ Bloco inválido rejeitado!\n";
            return;
        }
    }

    chain.push_back(block);
}

void Blockchain::mineBlock(std::string minerAddress) {
    difficulty = adjustDifficulty();

    double reward = getBlockReward(chain.size());

    Transaction coinbase({}, { {minerAddress, reward} });

    Block newBlock(chain.size(), getLastBlock().hash, {coinbase});

    std::cout << "⛏️ Mining diff " << difficulty << "...\n";

    newBlock.mine(difficulty);

    addBlock(newBlock);

    totalSupply += reward;

    std::cout << "🎉 Reward: " << reward << "\n";
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

double Blockchain::getBalance(std::string address) {
    double balance = 0;

    for (auto &block : chain) {
        for (auto &tx : block.transactions) {
            for (auto &out : tx.vout) {
                if (out.address == address)
                    balance += out.amount;
            }
        }
    }

    return balance;
}

void Blockchain::send(std::string from, std::string to, double amount) {
    if (getBalance(from) < amount) {
        std::cout << "❌ Saldo insuficiente\n";
        return;
    }

    Transaction tx({}, { {to, amount} });

    Block newBlock(chain.size(), getLastBlock().hash, {tx});

    newBlock.mine(difficulty);

    addBlock(newBlock);

    std::cout << "✅ Transação enviada\n";
}

const std::vector<Block>& Blockchain::getChain() const {
    return chain;
}

void Blockchain::clearChain() {
    chain.clear();
}

bool Blockchain::isChainValid() {
    for (size_t i = 1; i < chain.size(); i++) {
        if (!isValidBlock(chain[i], chain[i - 1]))
            return false;
    }
    return true;
}
