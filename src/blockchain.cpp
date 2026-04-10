#include "../include/blockchain.h"
#include "../include/storage.h"
#include <iostream>
#include <chrono>

Blockchain::Blockchain() {
    difficulty = 5;
    totalSupply = 0;
}

Block Blockchain::getLastBlock() {
    if (chain.empty()) return Block(-1, "0", {});
    return chain.back();
}

double Blockchain::getBlockReward(int height) {
    if (totalSupply >= 20000000) return 0;
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

    if (timeTaken < 1) timeTaken = 1;

    std::cout << "\n📊 --- AJUSTE DE DIFICULDADE ---" << std::endl;
    if (timeTaken < timeExpected / 2) {
        difficulty++;
        std::cout << "🔥 Rede rápida! Nova Dificuldade: " << difficulty << std::endl;
    } 
    else if (timeTaken > timeExpected * 2) {
        if (difficulty > 1) difficulty--;
        std::cout << "🧊 Rede lenta! Nova Dificuldade: " << difficulty << std::endl;
    } else {
        std::cout << "✅ Estável. Mantendo Diff: " << difficulty << std::endl;
    }
    std::cout << "--------------------------------\n" << std::endl;
}

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

    std::cout << "⛏️ Minando bloco " << newBlock.index << " (Diff: " << difficulty << ")..." << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    newBlock.mine(difficulty);
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> diff = end - start;
    std::cout << "🚀 Velocidade: " << (newBlock.nonce / diff.count()) / 1000 << " KH/s" << std::endl;

    chain.push_back(newBlock);
    totalSupply += reward;
    Storage::saveChain(*this, "data/blockchain.dat");
}

double Blockchain::getBalance(std::string address) {
    double balance = 0;
    for (const auto &block : chain) {
        for (const auto &tx : block.transactions) {
            for (const auto &out : tx.vout) {
                if (out.address == address) balance += out.amount;
            }
            // Simplificação: se o endereço enviou, o valor saiu via transação (TX)
            for (const auto &in : tx.vin) {
                // Aqui entraria a lógica de UTXO real, mas para o seu MVP:
                // Se 'from' está numa transação, o valor foi debitado.
            }
        }
    }
    return balance;
}

void Blockchain::send(std::string from, std::string to, double amount) {
    double fee = amount * 0.01; // Taxa de 1%
    if (getBalance(from) < (amount + fee)) {
        std::cout << "❌ Saldo insuficiente (+ 1% taxa)\n";
        return;
    }

    // Transação de envio + Troco (subtraindo o enviado e a taxa)
    Transaction tx({}, { {to, amount}, {from, getBalance(from) - (amount + fee)} });
    
    // O minerador (miner1) ganha a recompensa do bloco + a taxa (fee)
    double rewardWithFee = getBlockReward(chain.size()) + fee;
    Transaction coinbase({}, { {"miner1", rewardWithFee} });

    Block newBlock(chain.size(), getLastBlock().hash, {tx, coinbase});
    newBlock.mine(difficulty);
    chain.push_back(newBlock);
    Storage::saveChain(*this, "data/blockchain.dat");
    std::cout << "✅ Transferência de " << amount << " confirmada!\n";
}

std::vector<Block> Blockchain::getChain() const { return chain; }
int Blockchain::getDifficulty() const { return difficulty; }
void Blockchain::setDifficulty(int d) { difficulty = d; }
void Blockchain::clearChain() { chain.clear(); totalSupply = 0; }
void Blockchain::addBlock(const Block& block) {
    chain.push_back(block);
    for(const auto& tx : block.transactions)
        for(const auto& out : tx.vout) totalSupply += out.amount;
}
