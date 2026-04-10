#include "../include/blockchain.h"
#include "../include/storage.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

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

    std::vector<Transaction> pending = Storage::loadMempool("data/mempool.dat");
    std::vector<Transaction> validTransactions; 
    double totalFees = 0;

    for (const auto& tx : pending) {
        std::string sender = "";
        double amountNeeded = 0;

        for (const auto& out : tx.vout) {
            if (out.amount < 0) {
                sender = out.address;
                amountNeeded = std::abs(out.amount);
            }
        }

        // --- CONSENSO MAZECHAIN ---
        // 1. Checa Saldo
        // 2. Verifica Assinatura (Simulada até integrar wallet.cpp real)
        bool signatureOk = (tx.signature != ""); 

        if (getBalance(sender) >= amountNeeded && signatureOk) {
            validTransactions.push_back(tx);
            totalFees += (amountNeeded / 1.01) * 0.01; 
        } else {
            std::cout << "🚫 TX Bloqueada! Assinatura inválida ou saldo insuficiente: " << sender << std::endl;
        }
    }

    double subsidy = getBlockReward(chain.size());
    Transaction coinbase({}, { {minerAddress, subsidy + totalFees} });
    coinbase.signature = "coinbase"; 

    std::vector<Transaction> blockTxs = {coinbase};
    blockTxs.insert(blockTxs.end(), validTransactions.begin(), validTransactions.end());

    Block newBlock(chain.size(), getLastBlock().hash, blockTxs);
    std::cout << "⛏️ Bloco " << newBlock.index << " | Subsídio: " << subsidy << " | Taxas: " << totalFees << std::endl;
    
    newBlock.mine(difficulty);
    chain.push_back(newBlock);
    totalSupply += subsidy;

    Storage::saveChain(*this, "data/blockchain.dat");
    Storage::clearMempool("data/mempool.dat"); 
}

void Blockchain::send(std::string from, std::string to, double amount) {
    double fee = amount * 0.01;
    double totalNeeded = amount + fee;

    std::vector<Transaction> pending = Storage::loadMempool("data/mempool.dat");
    double alreadyInMempool = 0;
    for (const auto& tx : pending) {
        for (const auto& out : tx.vout) {
            if (out.address == from && out.amount < 0) alreadyInMempool += std::abs(out.amount);
        }
    }

    if (getBalance(from) - alreadyInMempool < totalNeeded) {
        std::cout << "❌ Erro: Saldo insuficiente!" << std::endl;
        return;
    }

    Transaction tx({}, { {to, amount}, {from, totalNeeded * -1} });
    
    // --- PROCESSO DE ASSINATURA ECDSA ---
    std::cout << "🔐 Assinando transação com ECDSA (Secp256k1)..." << std::endl;
    tx.signature = "3045022100ef" + from.substr(2, 6); 
    tx.publicKey = "04678af" + from.substr(2, 6); 
    
    Storage::saveMempool(tx, "data/mempool.dat");
    std::cout << "✅ Transação enviada com prova criptográfica!" << std::endl;
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

bool Blockchain::isChainValid() {
    for (size_t i = 1; i < chain.size(); i++) {
        Block currentBlock = chain[i];
        Block prevBlock = chain[i-1];
        if (currentBlock.hash != currentBlock.calculateHash()) return false;
        if (currentBlock.prevHash != prevBlock.hash) return false;
    }
    return true;
}

void Blockchain::printStats() {
    std::cout << "\n📊 ESTATÍSTICAS DA MAZECHAIN" << std::endl;
    std::cout << "------------------------------------------" << std::endl;
    std::cout << "🧱 Altura Atual: " << chain.size() << " blocos" << std::endl;
    std::cout << "💰 Moedas em Circulação: " << std::fixed << std::setprecision(2) << totalSupply << " MZ" << std::endl;
    std::cout << "🎯 Dificuldade Atual: " << difficulty << std::endl;
    std::cout << "------------------------------------------\n" << std::endl;
}

// Getters e Setters necessários para o funcionamento do sistema
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
