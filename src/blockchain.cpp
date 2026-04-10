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
    int interval = 10; 
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

    // --- LOGICA DE PROTEÇÃO E LIMPEZA DA MEMPOOL ---
    std::vector<Transaction> pending = Storage::loadMempool("data/mempool.dat");
    std::vector<Transaction> validTransactions; 
    double totalFees = 0;

    for (const auto& tx : pending) {
        std::string sender = "";
        double amountNeeded = 0;

        // Identifica quem está enviando e quanto (valor negativo no seu modelo)
        for (const auto& out : tx.vout) {
            if (out.amount < 0) {
                sender = out.address;
                amountNeeded = std::abs(out.amount);
            }
        }

        // VALIDAÇÃO REAL: O minerador checa o saldo atual antes de aceitar a TX
        if (getBalance(sender) >= amountNeeded) {
            validTransactions.push_back(tx);
            // Taxa de 1% sobre o valor enviado (o valor total inclui a taxa)
            // No seu modelo: totalNeeded = amount + fee, então a taxa é (totalNeeded / 1.01) * 0.01
            totalFees += (amountNeeded / 1.01) * 0.01; 
        } else {
            std::cout << "🚫 TX Rejeitada! " << sender << " tentou gastar sem ter saldo." << std::endl;
        }
    }
    // ----------------------------------------------

    double subsidy = getBlockReward(chain.size());
    Transaction coinbase({}, { {minerAddress, subsidy + totalFees} });

    std::vector<Transaction> blockTxs = {coinbase};
    blockTxs.insert(blockTxs.end(), validTransactions.begin(), validTransactions.end());

    Block newBlock(chain.size(), getLastBlock().hash, blockTxs);
    std::cout << "⛏️ Bloco " << newBlock.index << " | Subsídio: " << subsidy << " | Taxas: " << totalFees << std::endl;
    
    newBlock.mine(difficulty);
    chain.push_back(newBlock);
    totalSupply += subsidy;

    Storage::saveChain(*this, "data/blockchain.dat");
    Storage::clearMempool("data/mempool.dat"); // Limpa tudo, as rejeitadas somem da rede
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
    double totalNeeded = amount + fee;

    // Verificação preventiva básica
    if (getBalance(from) < totalNeeded) {
        std::cout << "❌ Erro: Saldo insuficiente!" << std::endl;
        return;
    }

    Transaction tx({}, { {to, amount}, {from, totalNeeded * -1} });
    Storage::saveMempool(tx, "data/mempool.dat");
    std::cout << "✅ Transação enviada para Mempool!" << std::endl;
}

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

bool Blockchain::isChainValid() {
    for (size_t i = 1; i < chain.size(); i++) {
        Block currentBlock = chain[i];
        Block prevBlock = chain[i-1];
        if (currentBlock.hash != currentBlock.calculateHash()) return false;
        if (currentBlock.prevHash != prevBlock.hash) return false;
        std::string target(difficulty, '0');
        if (currentBlock.hash.substr(0, difficulty) != target) return false;
    }
    return true;
}

void Blockchain::printStats() {
    std::cout << "\n📊 ESTATÍSTICAS DA MAZECHAIN" << std::endl;
    std::cout << "------------------------------------------" << std::endl;
    std::cout << "🧱 Altura Atual: " << chain.size() << " blocos" << std::endl;
    std::cout << "💰 Moedas em Circulação: " << std::fixed << std::setprecision(2) << totalSupply << " MZ" << std::endl;
    std::cout << "🎯 Dificuldade Atual: " << difficulty << std::endl;

    if (chain.size() >= 2) {
        long totalTime = chain.back().timestamp - chain[0].timestamp;
        double avgTime = (double)totalTime / (chain.size() - 1);
        std::cout << "⏱️  Tempo Médio por Bloco: " << avgTime << " seg" << std::endl;
        
        double hashrate = std::pow(16, difficulty) / TARGET_BLOCK_TIME;
        std::cout << "⚡ Hashrate Estimado: " << hashrate << " H/s" << std::endl;
        
        if (avgTime < TARGET_BLOCK_TIME / 2) std::cout << "🔥 Status: Rede Rápida (Dificuldade deve subir)" << std::endl;
        else if (avgTime > TARGET_BLOCK_TIME * 2) std::cout << "❄️  Status: Rede Lenta (Dificuldade deve cair)" << std::endl;
        else std::cout << "🟢 Status: Rede Estável" << std::endl;
    }
    std::cout << "------------------------------------------\n" << std::endl;
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
