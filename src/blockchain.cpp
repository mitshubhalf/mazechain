#include "../include/blockchain.h"
#include "../include/storage.h"
#include <iostream>
#include <chrono>
#include <algorithm>

Blockchain::Blockchain() {
    difficulty = 5;
    totalSupply = 0;
}

Block Blockchain::getLastBlock() {
    if (chain.empty()) return Block(-1, "0", {});
    return chain.back();
}

// Retorna a recompensa atual (Halving a cada 1000 blocos)
double Blockchain::getBlockReward(int height) {
    if (totalSupply >= 20000000) return 0;
    double reward = 250.0;
    int halvings = height / 1000;
    for (int i = 0; i < halvings; i++) reward /= 2.0;
    return reward;
}

// Ajuste de dificuldade estilo Bitcoin (Intervalo de 10 blocos)
void Blockchain::adjustDifficulty() {
    if (chain.size() < DIFFICULTY_ADJUSTMENT_INTERVAL) return;

    const Block& lastBlock = chain.back();
    const Block& relayBlock = chain[chain.size() - DIFFICULTY_ADJUSTMENT_INTERVAL];

    long timeExpected = TARGET_BLOCK_TIME * DIFFICULTY_ADJUSTMENT_INTERVAL;
    long timeTaken = lastBlock.timestamp - relayBlock.timestamp;

    if (timeTaken < 1) timeTaken = 1;

    std::cout << "\n📊 --- AJUSTE DE DIFICULDADE ---" << std::endl;
    std::cout << "Tempo gasto: " << timeTaken << "s | Esperado: " << timeExpected << "s" << std::endl;

    if (timeTaken < timeExpected / 2) {
        difficulty++;
        std::cout << "🔥 Rede muito rápida! Dificuldade subiu para: " << difficulty << std::endl;
    } 
    else if (timeTaken > timeExpected * 2) {
        if (difficulty > 1) difficulty--;
        std::cout << "🧊 Rede muito lenta! Dificuldade caiu para: " << difficulty << std::endl;
    } else {
        std::cout << "✅ Estabilidade detectada. Mantendo Diff: " << difficulty << std::endl;
    }
    std::cout << "--------------------------------\n" << std::endl;
}

// Mineração de blocos padrão (Gera novas moedas)
void Blockchain::mineBlock(std::string minerAddress) {
    if (chain.empty()) {
        Block genesis(0, "0", {});
        genesis.mine(difficulty);
        chain.push_back(genesis);
    }

    if (chain.size() % DIFFICULTY_ADJUSTMENT_INTERVAL == 0) adjustDifficulty();

    double reward = getBlockReward(chain.size());
    Transaction coinbase({}, { {minerAddress, reward} });
    Block newBlock(chain.size(), getLastBlock().hash, {coinbase});

    std::cout << "⛏️ Minerando bloco " << newBlock.index << " (Diff: " << difficulty << ")..." << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    newBlock.mine(difficulty);
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> diff = end - start;
    std::cout << "🚀 Velocidade: " << (newBlock.nonce / diff.count()) / 1000 << " KH/s" << std::endl;

    chain.push_back(newBlock);
    totalSupply += reward;
    Storage::saveChain(*this, "data/blockchain.dat");
}

// Cálculo de Saldo (Soma entradas - Soma saídas)
double Blockchain::getBalance(std::string address) {
    double balance = 0;
    for (const auto &block : chain) {
        for (const auto &tx : block.transactions) {
            for (const auto &out : tx.vout) {
                if (out.address == address) balance += out.amount;
            }
        }
    }
    return balance;
}

// Envio de moedas (Lógica de Taxas Corrigida)
void Blockchain::send(std::string from, std::string to, double amount) {
    double fee = amount * 0.01; // Taxa fixa de 1%
    double totalCost = amount + fee;
    double currentBalance = getBalance(from);

    if (currentBalance < totalCost) {
        std::cout << "❌ Saldo insuficiente! Saldo: " << currentBalance << " | Necessário (c/ taxa): " << totalCost << "\n";
        return;
    }

    // Registra a transferência e o débito negativo para o remetente
    Transaction tx({}, { 
        {to, amount}, 
        {from, (totalCost) * -1} 
    });

    std::cout << "📦 Transação validada. Confirmando no bloco..." << std::endl;

    // O minerador ganha apenas a taxa (FEE), sem gerar inflação (Reward 0)
    Transaction feeReward({}, { {"miner1", fee} }); 

    Block newBlock(chain.size(), getLastBlock().hash, {tx, feeReward});
    newBlock.mine(difficulty);
    
    chain.push_back(newBlock);
    Storage::saveChain(*this, "data/blockchain.dat");
    
    std::cout << "✅ Transferência de " << amount << " confirmada! Taxa: " << fee << "\n";
}

// Getters e Setters de suporte
std::vector<Block> Blockchain::getChain() const { return chain; }
int Blockchain::getDifficulty() const { return difficulty; }
void Blockchain::setDifficulty(int d) { difficulty = d; }
void Blockchain::clearChain() { chain.clear(); totalSupply = 0; }

void Blockchain::addBlock(const Block& block) {
    chain.push_back(block);
    for(const auto& tx : block.transactions) {
        for(const auto& out : tx.vout) {
            // Apenas vouts positivos aumentam o suprimento total nominal
            if(out.amount > 0) totalSupply += out.amount;
        }
    }
}
