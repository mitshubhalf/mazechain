#include "../include/blockchain.h"
#include "../include/storage.h"
#include <iostream>

Blockchain::Blockchain() {
    difficulty = 5; // Começa na dificuldade 5
    totalSupply = 0;
}

Block Blockchain::getLastBlock() {
    if (chain.empty()) return Block(-1, "0", {});
    return chain.back();
}

double Blockchain::getBlockReward(int height) {
    if (totalSupply >= 20000000) return 0; // Limite de 20 Milhões

    double reward = 250.0;
    int halvings = height / 1000;
    for (int i = 0; i < halvings; i++) reward /= 2.0;
    
    return reward;
}

void Blockchain::adjustDifficulty() {
    if (chain.size() < DIFFICULTY_ADJUSTMENT_INTERVAL) return;

    const Block& lastBlock = chain.back();
    const Block& relayBlock = chain[chain.size() - DIFFICULTY_ADJUSTMENT_INTERVAL];

    long timeExpected = TARGET_BLOCK_TIME * DIFFICULTY_ADJUSTMENT_INTERVAL;
    long timeTaken = lastBlock.timestamp - relayBlock.timestamp;

    std::cout << "\n📊 --- AJUSTE DE DIFICULDADE ---" << std::endl;
    std::cout << "Tempo gasto: " << timeTaken << "s | Esperado: " << timeExpected << "s" << std::endl;

    if (timeTaken < timeExpected / 2) {
        difficulty++;
        std::cout << "🔥 Rede rápida! Dificuldade subiu para: " << difficulty << std::endl;
    } 
    else if (timeTaken > timeExpected * 2) {
        if (difficulty > 1) difficulty--;
        std::cout << "🧊 Rede lenta! Dificuldade caiu para: " << difficulty << std::endl;
    }
    std::cout << "--------------------------------\n" << std::endl;
}

void Blockchain::mineBlock(std::string minerAddress) {
    if (chain.empty()) {
        Block genesis(0, "0", {});
        genesis.mine(difficulty);
        chain.push_back(genesis);
    }

    // Ajusta a dificuldade a cada 10 blocos
    if (chain.size() % DIFFICULTY_ADJUSTMENT_INTERVAL == 0) {
        adjustDifficulty();
    }

    double reward = getBlockReward(chain.size());
    Transaction coinbase({}, { {minerAddress, reward} });
    Block newBlock(chain.size(), getLastBlock().hash, {coinbase});

    std::cout << "⛏️ Mining block " << newBlock.index << " (Diff: " << difficulty << ")..." << std::endl;
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

std::vector<Block> getChain() const { return chain; }
int getDifficulty() const { return difficulty; }
void clearChain() { chain.clear(); totalSupply = 0; }

void Blockchain::addBlock(const Block& block) {
    chain.push_back(block);
    for(const auto& tx : block.transactions)
        for(const auto& out : tx.vout) totalSupply += out.amount;
}
