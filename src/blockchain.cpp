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

// --- UTILITÁRIOS ---
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

std::string calculateMerkleRoot(const std::vector<Transaction>& txs) {
    if (txs.empty()) return sha256_util("empty");
    
    std::vector<std::string> tree;
    for (const auto& tx : txs) {
        tree.push_back(tx.id);
    }
    
    while (tree.size() > 1) {
        if (tree.size() % 2 != 0) {
            tree.push_back(tree.back());
        }
        std::vector<std::string> newLevel;
        for (size_t i = 0; i < tree.size(); i += 2) {
            newLevel.push_back(sha256_util(tree[i] + tree[i+1]));
        }
        tree = newLevel;
    }
    return tree[0];
}

// --- IMPLEMENTAÇÃO DA CLASSE BLOCK ---
Block::Block(int idx, std::string prev, std::vector<Transaction> txs) {
    index = idx;
    prevHash = prev;
    transactions = txs;
    timestamp = std::time(0); 
    nonce = 0;
    hash = calculateHash();
}

std::string Block::calculateHash() const {
    std::stringstream ss;
    std::string root = calculateMerkleRoot(this->transactions);
    // Inclui timestamp e root para garantir unicidade do hash
    ss << index << (long long)timestamp << prevHash << nonce << root;
    return sha256_util(ss.str());
}

void Block::mine(int difficulty) {
    std::string target(difficulty, '0');
    // Loop de mineração (Proof of Work)
    while (hash.substr(0, difficulty) != target) {
        nonce++;
        hash = calculateHash();
    }
    std::cout << "🎯 Bloco Minerado! Hash: " << hash << std::endl;
}

// --- IMPLEMENTAÇÃO DA CLASSE BLOCKCHAIN ---
Blockchain::Blockchain() {
    difficulty = 4; // Dificuldade padrão
    totalSupply = 0;
    
    // Cria bloco gênese se a corrente estiver vazia
    if (chain.empty()) {
        Block genesis(0, "0", {});
        genesis.mine(difficulty);
        chain.push_back(genesis);
    }
}

bool Blockchain::verifyTransaction(const Transaction& tx) {
    if (tx.signature == "coinbase") return true;
    if (tx.signature.empty() || tx.publicKey.empty()) return false;

    std::string cleanKey = tx.publicKey;
    cleanKey.erase(0, cleanKey.find_first_not_of(" \t\r\n"));
    cleanKey.erase(cleanKey.find_last_not_of(" \t\r\n") + 1);

    std::string senderAddress = "";
    for (const auto& out : tx.vout) {
        if (out.amount < 0) senderAddress = out.address;
    }

    std::string expectedAddress = "MZ" + sha256_util(cleanKey).substr(0, 20);
    return (senderAddress == expectedAddress);
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
    adjustDifficulty();

    std::vector<Transaction> pending = Storage::loadMempool("data/mempool.dat");
    std::vector<Transaction> validTransactions; 
    double totalFees = 0;
    std::map<std::string, double> spendingInThisBlock;

    for (const auto& tx : pending) {
        if (!verifyTransaction(tx)) {
            std::cout << "⚠️ Transação ignorada: Assinatura inválida." << std::endl;
            continue;
        }

        std::string sender = "";
        double amountWithFee = 0;
        for (const auto& out : tx.vout) { 
            if (out.amount < 0) { sender = out.address; amountWithFee = std::abs(out.amount); } 
        }

        double currentBalance = getBalance(sender) - spendingInThisBlock[sender];

        if (currentBalance >= amountWithFee) {
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
    
    chain.push_back(newBlock);
    totalSupply += subsidy;

    // Salva automaticamente após minerar
    Storage::saveChain(*this, "data/blockchain.dat");
    Storage::clearMempool("data/mempool.dat"); 
}

void Blockchain::send(std::string from, std::string to, double amount, std::string seed) {
    double totalNeeded = amount * 1.01;
    if (getBalance(from) < totalNeeded) { 
        std::cout << "❌ Saldo insuficiente!" << std::endl; 
        return; 
    }

    std::string finalSeed = seed;
    if (finalSeed.empty()) {
        std::cout << "🔒 Digite sua SEED (12 palavras) para assinar esta transação: ";
        std::getline(std::cin >> std::ws, finalSeed);
    }

    Transaction tx;
    tx.id = sha256_util(from + to + std::to_string(amount) + std::to_string(std::time(0)));
    tx.vout.push_back({to, amount});
    tx.vout.push_back({from, totalNeeded * -1});
    
    tx.publicKey = finalSeed; 
    tx.signature = "SIG_" + sha256_util(finalSeed).substr(0, 16); 
    
    Storage::saveMempool(tx, "data/mempool.dat");
    std::cout << "✅ Transação assinada e enviada para Mempool!" << std::endl;
}

double Blockchain::getBalance(std::string address) {
    double balance = 0;
    for (const auto &block : chain) 
        for (const auto &tx : block.transactions) 
            for (const auto &out : tx.vout) 
                if (out.address == address) balance += out.amount;
    return balance;
}

bool Blockchain::isChainValid(const std::vector<Block>& chainToValidate) {
    for (size_t i = 1; i < chainToValidate.size(); i++) {
        const Block& currentBlock = chainToValidate[i];
        const Block& prevBlock = chainToValidate[i-1];

        if (currentBlock.hash != currentBlock.calculateHash()) return false;
        if (currentBlock.prevHash != prevBlock.hash) return false;
        
        for (const auto& tx : currentBlock.transactions) {
            if (!verifyTransaction(tx)) return false;
        }
    }
    return true;
}

bool Blockchain::isChainValid() {
    return isChainValid(this->chain);
}

void Blockchain::replaceChain(const std::vector<Block>& newChain) {
    if (newChain.size() > chain.size() && isChainValid(newChain)) {
        chain = newChain;
        std::cout << "✅ Sincronizado: Corrente local atualizada com a rede." << std::endl;
        Storage::saveChain(*this, "data/blockchain.dat");
    }
}

void Blockchain::printStats() {
    std::cout << "\n📊 Altura: " << chain.size() << " | 💰 Circulação: " << std::fixed << std::setprecision(2) << totalSupply << " MZ" << std::endl;
}

std::vector<Block> Blockchain::getChain() const { return chain; }
int Blockchain::getDifficulty() const { return difficulty; }
void Blockchain::setDifficulty(int d) { difficulty = d; }
void Blockchain::clearChain() { chain.clear(); totalSupply = 0; }
void Blockchain::addBlock(const Block& block) { 
    chain.push_back(block); 
    if(!block.transactions.empty()) {
        for(const auto& tx : block.transactions) {
            if(tx.signature == "coinbase") {
                for(const auto& out : tx.vout) totalSupply += out.amount;
            }
        }
    }
}
