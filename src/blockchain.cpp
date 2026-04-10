#include "../include/blockchain.h"
#include "../include/storage.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <iomanip>

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
    int interval = 10; // Mantido em 10 conforme seu teste de sucesso
    int h = height;

    while (h >= interval) {
        reward /= 2.0;
        h -= interval;
        interval *= 2; 
    }
    return (reward < 0.000001) ? 0 : reward;
}

void Blockchain::adjustDifficulty() {
    if (chain.size() < DIFFICULTY_ADJUSTMENT_INTERVAL) return;
    const Block& lastBlock = chain.back();
    const Block& relayBlock = chain[chain.size() - DIFFICULTY_ADJUSTMENT_INTERVAL];
    long timeExpected = TARGET_BLOCK_TIME * DIFFICULTY_ADJUSTMENT_INTERVAL;
    long timeTaken = lastBlock.timestamp - relayBlock.timestamp;
    if (timeTaken < 1) timeTaken = 1;

    if (timeTaken < timeExpected / 2) difficulty++;
    else if (timeTaken > timeExpected * 2 && difficulty > 1) difficulty--;
}

void Blockchain::mineBlock(std::string minerAddress) {
    if (chain.empty()) {
        Block genesis(0, "0", {});
        genesis.mine(difficulty);
        chain.push_back(genesis);
    }
    if (chain.size() % DIFFICULTY_ADJUSTMENT_INTERVAL == 0) adjustDifficulty();

    std::vector<Transaction> pending = Storage::loadMempool("data/mempool.dat");
    double totalFees = 0;
    for (const auto& tx : pending) {
        for (const auto& out : tx.vout) {
            if (out.amount > 0) totalFees += (out.amount * 0.01);
        }
    }

    double subsidy = getBlockReward(chain.size());
    Transaction coinbase({}, { {minerAddress, subsidy + totalFees} });

    std::vector<Transaction> blockTxs = {coinbase};
    blockTxs.insert(blockTxs.end(), pending.begin(), pending.end());

    Block newBlock(chain.size(), getLastBlock().hash, blockTxs);
    std::cout << "⛏️ Bloco " << newBlock.index << " | Subsídio: " << subsidy << " | Taxas: " << totalFees << std::endl;
    
    newBlock.mine(difficulty);
    chain.push_back(newBlock);
    totalSupply += subsidy;

    Storage::saveChain(*this, "data/blockchain.dat");
    Storage::clearMempool("data/mempool.dat");
}

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

void Blockchain::send(std::string from, std::string to, double amount) {
    double fee = amount * 0.01;
    if (getBalance(from) < (amount + fee)) {
        std::cout << "❌ Saldo insuficiente!" << std::endl;
        return;
    }
    Transaction tx({}, { {to, amount}, {from, (amount + fee) * -1} });
    Storage::saveMempool(tx, "data/mempool.dat");
    std::cout << "✅ Transação enviada para Mempool!" << std::endl;
}

// Nova função para detalhar o bloco
void Blockchain::printBlockDetails(int height) {
    if (height < 0 || height >= (int)chain.size()) {
        std::cout << "❌ Erro: Bloco " << height << " não encontrado!" << std::endl;
        return;
    }

    const Block& block = chain[height];
    std::cout << "\n==========================================" << std::endl;
    std::cout << "📦 DETALHES DO BLOCO #" << block.index << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "🔗 Hash: " << block.hash << std::endl;
    std::cout << "⬅️  Prev Hash: " << block.prevHash << std::endl;
    std::cout << "⏰ Timestamp: " << block.timestamp << std::endl;
    std::cout << "🔢 Nonce: " << block.nonce << std::endl;
    std::cout << "🧾 Transações (" << block.transactions.size() << "):" << std::endl;

    for (size_t i = 0; i < block.transactions.size(); ++i) {
        const auto& tx = block.transactions[i];
        std::cout << "   --------------------------------------" << std::endl;
        if (i == 0) std::cout << "   [COINBASE - Recompensa de Mineração]" << std::endl;
        
        for (const auto& out : tx.vout) {
            if (out.amount > 0)
                std::cout << "   ➡️  Para: " << out.address << " | Valor: +" << out.amount << " MZ" << std::endl;
            else
                std::cout << "   ⬅️  De:   " << out.address << " | Valor: " << out.amount << " MZ (Saída + Taxa)" << std::endl;
        }
    }
    std::cout << "==========================================\n" << std::endl;
}

std::vector<Block> Blockchain::getChain() const { return chain; }
int Blockchain::getDifficulty() const { return difficulty; }
void Blockchain::setDifficulty(int d) { difficulty = d; }
void Blockchain::clearChain() { chain.clear(); totalSupply = 0; }
void Blockchain::addBlock(const Block& block) {
    chain.push_back(block);
    if(!block.transactions.empty()){
        for(const auto& out : block.transactions[0].vout) 
            if(out.amount > 0) totalSupply += out.amount;
    }
}
