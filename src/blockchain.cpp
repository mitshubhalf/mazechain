#include "../include/blockchain.h"
#include "../include/storage.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <openssl/sha.h>
#include <map>
#include <ctime>

// --- UTILITÁRIO SHA256 ---
std::string sha256_util(std::string str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

// --- IMPLEMENTAÇÃO DA CLASSE BLOCK ---
Block::Block(int idx, std::string prev, std::vector<Transaction> txs) {
    index = idx;
    prevHash = prev;
    transactions = txs;
    // MELHORIA: Define o timestamp uma única vez na criação
    timestamp = std::time(0); 
    nonce = 0;
    hash = calculateHash();
}

std::string Block::calculateHash() const {
    std::stringstream ss;
    // MELHORIA: Cast para long long garante que o número seja tratado como texto fixo
    ss << index << (long long)timestamp << prevHash << nonce;
    for (const auto& tx : transactions) {
        ss << tx.id;
        for (const auto& out : tx.vout) {
            // MELHORIA: Precisão de 8 casas decimais evita erro de arredondamento no hash
            ss << out.address << std::fixed << std::setprecision(8) << out.amount;
        }
    }
    return sha256_util(ss.str());
}

void Block::mine(int difficulty) {
    std::string target(difficulty, '0');
    while (hash.substr(0, difficulty) != target) {
        nonce++;
        hash = calculateHash();
    }
    std::cout << "🎯 Bloco Minerado! Hash: " << hash << std::endl;
}

// --- IMPLEMENTAÇÃO DA CLASSE BLOCKCHAIN ---
Blockchain::Blockchain() {
    difficulty = 5;
    totalSupply = 0;
}

Block Blockchain::getLastBlock() {
    if (chain.empty()) return Block(-1, "0", {});
    return chain.back();
}

double Blockchain::getBlockReward(int height) {
    if (totalSupply >= 21000000) return 0;
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
    if (chain.size() < 10) return;
    const Block& lastBlock = chain.back();
    const Block& relayBlock = chain[chain.size() - 10];
    long timeExpected = 60 * 10;
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
    adjustDifficulty();

    std::vector<Transaction> pending = Storage::loadMempool("data/mempool.dat");
    std::vector<Transaction> validTransactions; 
    double totalFees = 0;
    std::map<std::string, double> spendingInThisBlock;

    for (const auto& tx : pending) {
        std::string sender = "";
        double amountWithFee = 0;
        for (const auto& out : tx.vout) { 
            if (out.amount < 0) { 
                sender = out.address; 
                amountWithFee = std::abs(out.amount); 
            } 
        }

        double currentBalance = getBalance(sender) - spendingInThisBlock[sender];

        if (currentBalance >= amountWithFee && !tx.signature.empty()) {
            validTransactions.push_back(tx);
            spendingInThisBlock[sender] += amountWithFee;
            totalFees += (amountWithFee / 1.01) * 0.01; 
        } else {
            std::cout << "⚠️ TX Ignorada: Saldo insuficiente para " << sender << std::endl;
        }
    }

    double subsidy = getBlockReward(chain.size());
    Transaction coinbase;
    coinbase.id = sha256_util("coinbase" + std::to_string(chain.size()) + minerAddress);
    coinbase.vout.push_back({minerAddress, subsidy + totalFees});
    coinbase.signature = "coinbase"; 

    std::vector<Transaction> blockTxs;
    blockTxs.push_back(coinbase);
    for(const auto& t : validTransactions) blockTxs.push_back(t);

    Block newBlock(chain.size(), chain.back().hash, blockTxs);
    newBlock.mine(difficulty);
    
    chain.push_back(newBlock);
    totalSupply += subsidy;

    Storage::saveChain(*this, "data/blockchain.dat");
    Storage::clearMempool("data/mempool.dat"); 
}

void Blockchain::send(std::string from, std::string to, double amount) {
    double totalNeeded = amount * 1.01;
    if (getBalance(from) < totalNeeded) { 
        std::cout << "❌ Saldo insuficiente!" << std::endl; 
        return; 
    }

    Transaction tx;
    tx.id = sha256_util(from + to + std::to_string(amount) + std::to_string(std::time(0)));
    tx.vout.push_back({to, amount});
    tx.vout.push_back({from, totalNeeded * -1});
    tx.signature = "SIG_AUTH_" + from.substr(2, 6); 
    
    Storage::saveMempool(tx, "data/mempool.dat");
    std::cout << "✅ Transação enviada para Mempool!" << std::endl;
}

double Blockchain::getBalance(std::string address) {
    double balance = 0;
    for (const auto &block : chain) 
        for (const auto &tx : block.transactions) 
            for (const auto &out : tx.vout) 
                if (out.address == address) balance += out.amount;
    return balance;
}

bool Blockchain::isChainValid() {
    for (size_t i = 1; i < chain.size(); i++) {
        // Agora o hash calculado será idêntico ao hash salvo, pois o timestamp é fixo
        if (chain[i].hash != chain[i].calculateHash()) return false;
        if (chain[i].prevHash != chain[i-1].hash) return false;
    }
    return true;
}

void Blockchain::printStats() {
    std::cout << "\n📊 ESTATÍSTICAS DA MAZECHAIN" << std::endl;
    std::cout << "------------------------------------------" << std::endl;
    std::cout << "🧱 Altura: " << chain.size() << " | 💰 Circulação: " << std::fixed << std::setprecision(2) << totalSupply << " MZ" << std::endl;
    std::cout << "⚙️ Dificuldade: " << difficulty << std::endl;
    std::cout << "------------------------------------------\n" << std::endl;
}

std::vector<Block> Blockchain::getChain() const { return chain; }
int Blockchain::getDifficulty() const { return difficulty; }
void Blockchain::setDifficulty(int d) { difficulty = d; }
void Blockchain::clearChain() { chain.clear(); totalSupply = 0; }

void Blockchain::addBlock(const Block& block) { 
    chain.push_back(block); 
    // RECONSTRUÇÃO: Usa o prêmio teórico para evitar erros de soma com taxas
    if(!block.transactions.empty()) {
        totalSupply += getBlockReward(block.index);
    }
}
