#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/crypto.h"
#include <iostream>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <map>

Blockchain::Blockchain() {
    difficulty = 4;
    totalSupply = 0;
    if (chain.empty()) {
        Block genesis(0, "0", {});
        genesis.mine(difficulty);
        chain.push_back(genesis);
    }
}

bool Blockchain::verifyTransaction(const Transaction& tx) {
    if (tx.signature == "coinbase") return true;
    if (tx.signature.empty() || tx.publicKey.empty()) return false;

    std::string senderAddress = "";
    for (const auto& out : tx.vout) {
        if (out.amount < 0) senderAddress = out.address;
    }

    // Regra de Maturidade (100 blocos)
    int currentHeight = chain.size();
    for (const auto& block : chain) {
        for (const auto& blockTx : block.transactions) {
            if (blockTx.signature == "coinbase") {
                for (const auto& out : blockTx.vout) {
                    if (out.address == senderAddress) {
                        if (currentHeight - block.index < 1) {
                            std::cout << "⚠️ Bloqueio Coinbase Maturity: Aguarde 1 confirmações." << std::endl;
                            return false;
                        }
                    }
                }
            }
        }
    }

    std::string cleanKey = tx.publicKey;
    cleanKey.erase(0, cleanKey.find_first_not_of(" \t\r\n"));
    cleanKey.erase(cleanKey.find_last_not_of(" \t\r\n") + 1);
    std::string expectedAddress = "MZ" + Crypto::sha256_util(cleanKey).substr(0, 20);
    
    return (senderAddress == expectedAddress);
}

double Blockchain::getBlockReward(int height) {
    if (totalSupply >= 20000000) return 0;
    double reward = 1000.0;
    long interval = 1000;
    int h = height;
    while (h >= interval) {
        reward /= 2.0;
        h -= interval;
        interval *= 2;
        if (reward < 0.000001) return 0;
    }
    return reward;
}

void Blockchain::adjustDifficulty() {
    int height = chain.size();
    int n = getCurrentCycle(height);
    if (n < 5) return; 

    if (height < 10) return;
    const Block& lastBlock = chain.back();
    const Block& relayBlock = chain[height - 10];

    long timeTarget = 6000; 
    long timeTaken = lastBlock.timestamp - relayBlock.timestamp;
    if (timeTaken < 1) timeTaken = 1;

    if (timeTaken < timeTarget / 2) difficulty++;
    else if (timeTaken > timeTarget * 2 && difficulty > 1) difficulty--;
}

void Blockchain::mineBlock(std::string minerAddress) {
    if (minerAddress.substr(0, 2) != "MZ") return;
    adjustDifficulty();

    std::vector<Transaction> pending = Storage::loadMempool("data/mempool.dat");
    std::vector<Transaction> validTransactions;
    double totalFees = 0;
    std::map<std::string, double> spendingInThisBlock;

    for (const auto& tx : pending) {
        if (!verifyTransaction(tx)) continue;
        std::string sender = "";
        double amountWithFee = 0;
        for (const auto& out : tx.vout) {
            if (out.amount < 0) { sender = out.address; amountWithFee = std::abs(out.amount); }
        }

        if (getBalance(sender) - spendingInThisBlock[sender] >= amountWithFee) {
            validTransactions.push_back(tx);
            spendingInThisBlock[sender] += amountWithFee;
            totalFees += (amountWithFee / 1.01) * 0.01;
        }
    }

    double subsidy = getBlockReward(chain.size());
    Transaction coinbase;
    coinbase.id = "coinbase_" + std::to_string(chain.size());
    coinbase.vout.push_back({minerAddress, subsidy + totalFees});
    coinbase.signature = "coinbase";

    std::vector<Transaction> blockTxs = {coinbase};
    for(const auto& t : validTransactions) blockTxs.push_back(t);

    Block newBlock(chain.size(), chain.back().hash, blockTxs);
    newBlock.mine(difficulty);
    
    addBlock(newBlock);
    Storage::saveChain(*this, "data/blockchain.dat");
    Storage::clearMempool("data/mempool.dat");
}

double Blockchain::getBalance(std::string address) {
    double balance = 0;
    for (const auto &block : chain)
        for (const auto &tx : block.transactions)
            for (const auto &out : tx.vout)
                if (out.address == address) balance += out.amount;
    return balance;
}

int Blockchain::getCurrentCycle(int height) {
    int n = 0; long tempInterval = 1000; int tempHeight = height;
    while(tempHeight >= tempInterval) {
        tempHeight -= tempInterval;
        tempInterval *= 2;
        n++;
    }
    return n;
}

void Blockchain::addBlock(const Block& block) {
    chain.push_back(block);
    for(const auto& tx : block.transactions) {
        if(tx.signature == "coinbase") {
            for(const auto& out : tx.vout) totalSupply += out.amount;
        }
    }
}

void Blockchain::send(std::string from, std::string to, double amount, std::string seed) {
    double totalNeeded = amount * 1.01;
    if (getBalance(from) < totalNeeded) {
        std::cout << "❌ Saldo insuficiente!" << std::endl;
        return;
    }
    Transaction tx;
    tx.id = Crypto::sha256_util(from + to + std::to_string(amount) + std::to_string(std::time(0)));
    tx.vout.push_back({to, amount});
    tx.vout.push_back({from, totalNeeded * -1});
    tx.publicKey = seed;
    tx.signature = "SIG_" + Crypto::sha256_util(seed).substr(0, 16);
    
    Storage::saveMempool(tx, "data/mempool.dat");
}

bool Blockchain::isChainValid() {
    for (size_t i = 1; i < chain.size(); i++) {
        if (chain[i].hash != chain[i].calculateHash()) return false;
        if (chain[i].prevHash != chain[i-1].hash) return false;
    }
    return true;
}

void Blockchain::clearChain() { chain.clear(); totalSupply = 0; }
std::vector<Block> Blockchain::getChain() const { return chain; }
int Blockchain::getDifficulty() const { return difficulty; }
std::vector<Transaction> Blockchain::getMempool() const { return Storage::loadMempool("data/mempool.dat"); }

void Blockchain::printStats() {
    int height = chain.size();
    std::cout << "\n📊 Altura: " << height << " | Dificuldade: " << difficulty 
              << " | Circulação: " << std::fixed << std::setprecision(2) << totalSupply << " MZ" << std::endl;
}
