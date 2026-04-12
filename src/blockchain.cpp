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

// --- UTILITÁRIOS MANTIDOS ---
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
    for (const auto& tx : txs) tree.push_back(tx.id);
    while (tree.size() > 1) {
        if (tree.size() % 2 != 0) tree.push_back(tree.back());
        std::vector<std::string> newLevel;
        for (size_t i = 0; i < tree.size(); i += 2) {
            newLevel.push_back(sha256_util(tree[i] + tree[i+1]));
        }
        tree = newLevel;
    }
    return tree[0];
}

// --- CLASSE BLOCK MANTIDA ---
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
    ss << index << (long long)timestamp << prevHash << nonce << root;
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

// --- CLASSE BLOCKCHAIN INTEGRADA ---
Blockchain::Blockchain() {
    difficulty = 4;
    totalSupply = 0;
    if (chain.empty()) {
        Block genesis(0, "0", {});
        genesis.mine(difficulty);
        chain.push_back(genesis);
    }
}

// SINCRONIZADO: Verificação com Regra de Maturidade (100 blocos)
bool Blockchain::verifyTransaction(const Transaction& tx) {
    if (tx.signature == "coinbase") return true;
    if (tx.signature.empty() || tx.publicKey.empty()) return false;

    // Identifica o remetente
    std::string senderAddress = "";
    for (const auto& out : tx.vout) {
        if (out.amount < 0) senderAddress = out.address;
    }

    // REGRA DE MATURIDADE: Se os fundos vêm de uma coinbase, verifica a distância de 100 blocos
    int currentHeight = chain.size();
    for (const auto& block : chain) {
        for (const auto& blockTx : block.transactions) {
            if (blockTx.signature == "coinbase") {
                for (const auto& out : blockTx.vout) {
                    if (out.address == senderAddress) {
                        if (currentHeight - block.index < 100) {
                            std::cout << "⚠️ Bloqueio Coinbase Maturity: Aguarde 100 confirmações." << std::endl;
                            return false;
                        }
                    }
                }
            }
        }
    }

    // Verificação de Identidade (Padrão MazeChain)
    std::string cleanKey = tx.publicKey;
    cleanKey.erase(0, cleanKey.find_first_not_of(" \t\r\n"));
    cleanKey.erase(cleanKey.find_last_not_of(" \t\r\n") + 1);
    std::string expectedAddress = "MZ" + sha256_util(cleanKey).substr(0, 20);
    
    return (senderAddress == expectedAddress);
}

Block Blockchain::getLastBlock() {
    if (chain.empty()) return Block(-1, "0", {});
    return chain.back();
}

// SINCRONIZADO: Halving Progressivo (Dobro de blocos, metade da recompensa)
double Blockchain::getBlockReward(int height) {
    if (totalSupply >= 20000000) return 0; // Cap final opcional

    double reward = 1000.0;
    long interval = 1000; // Ciclo 0
    int h = height;

    while (h >= interval) {
        reward /= 2.0;       // Metade da recompensa
        h -= interval;       // Passa para o próximo ciclo
        interval *= 2;       // Dobra os blocos necessários
        if (reward < 0.000001) return 0;
    }
    return reward;
}

// SINCRONIZADO: Dificuldade (Tempo livre até o 5º Halving, depois 10 min alvo)
void Blockchain::adjustDifficulty() {
    int height = chain.size();
    
    // Cálculo do ciclo atual para saber se já passou do 5º halving
    int n = 0;
    long tempInterval = 1000;
    int tempHeight = height;
    while(tempHeight >= tempInterval) {
        tempHeight -= tempInterval;
        tempInterval *= 2;
        n++;
    }

    // Antes do 5º Halving: Rede livre. Após o 5º: Alvo de 10 min.
    if (n < 5) return; 

    if (height < 10) return;
    const Block& lastBlock = chain.back();
    const Block& relayBlock = chain[height - 10];

    long timeTarget = 6000; // 10 blocos * 10 minutos (600 segundos cada)
    long timeTaken = lastBlock.timestamp - relayBlock.timestamp;

    if (timeTaken < 1) timeTaken = 1;

    // nova_dificuldade = dificuldade_atual * (tempo_alvo / tempo_real)
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
    
    chain.push_back(newBlock);
    totalSupply += subsidy;

    Storage::saveChain(*this, "data/blockchain.dat");
    Storage::clearMempool("data/mempool.dat");
}

void Blockchain::send(std::string from, std::string to, double amount, std::string seed) {
    double totalNeeded = amount * 1.01;
    if (getBalance(from) < totalNeeded) {
        std::cout << "❌ Saldo insuficiente!" << std::endl;
        return;
    }

    Transaction tx;
    tx.id = sha256_util(from + to + std::to_string(amount) + std::to_string(std::time(0)));
    tx.vout.push_back({to, amount});
    tx.vout.push_back({from, totalNeeded * -1});
    tx.publicKey = seed;
    tx.signature = "SIG_" + sha256_util(seed).substr(0, 16);
    
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
        const Block& currentBlock = chain[i];
        const Block& prevBlock = chain[i-1];
        if (currentBlock.hash != currentBlock.calculateHash()) return false;
        if (currentBlock.prevHash != prevBlock.hash) return false;
        for (const auto& tx : currentBlock.transactions) {
            if (!verifyTransaction(tx)) return false;
        }
    }
    return true;
}

void Blockchain::printStats() {
    int height = chain.size();
    // Cálculo do Halving para o print
    int n = 0; long tempInterval = 1000; int tempHeight = height;
    while(tempHeight >= tempInterval) { tempHeight -= tempInterval; tempInterval *= 2; n++; }

    std::cout << "\n📊 Altura: " << height 
              << " | Ciclo Halving: " << n 
              << " | Dificuldade: " << difficulty 
              << " | Recompensa: " << getBlockReward(height) << " MZ"
              << " | Circulação: " << std::fixed << std::setprecision(2) << totalSupply << " MZ" << std::endl;
}

std::vector<Block> Blockchain::getChain() const { return chain; }
int Blockchain::getDifficulty() const { return difficulty; }
