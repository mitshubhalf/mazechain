#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/crypto.h"
#include <iostream>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <map>

// Constantes da Economia MazeChain
const double MITS_PER_COIN = 100000000.0; // 100 Milhões de Mits = 1 MZ

Blockchain::Blockchain() {
    difficulty = 4;
    totalSupply = 0;
    
    // Tenta carregar o estado anterior
    utxoSet.loadFromFile("data/utxo.dat");

    if (chain.empty()) {
        std::vector<Transaction> genesisTxs;
        Block genesis(0, "0", genesisTxs);
        genesis.hash = genesis.calculateHash(); 
        chain.push_back(genesis);
        std::cout << "[SISTEMA] Bloco Gênesis estabelecido." << std::endl;
    }
}

double Blockchain::getBlockReward(int height) {
    // MAX_SUPPLY agora vem do header como constante
    if (totalSupply >= getMaxSupply()) return 0.0;

    double reward = 1000.0;
    
    if (height > 0 && height <= 1000) {
        reward = 1000.0;
    } else if (height > 1000 && height <= 2000) {
        reward = 500.0;
    } else if (height > 2000 && height <= 4000) {
        reward = 250.0;
    } else if (height > 4000) {
        int halvings = (height - 4000) / 2000 + 3; 
        reward = 1000.0 / std::pow(2, halvings);
    }

    // Ajuste fino para não ultrapassar os 20M exatos
    if (totalSupply + reward > getMaxSupply()) {
        reward = getMaxSupply() - totalSupply;
    }

    return (reward < 0.00000001) ? 0.0 : reward;
}

void Blockchain::mineBlock(std::string minerAddress) {
    if (minerAddress.substr(0, 2) != "MZ") {
        std::cout << "❌ Endereço inválido!" << std::endl;
        return;
    }

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
            if (out.amount < 0) { 
                sender = out.address; 
                amountWithFee = std::abs(out.amount); 
            }
        }

        double currentBalance = getBalance(sender);
        // Proteção contra gasto duplo no mesmo bloco
        if (currentBalance - spendingInThisBlock[sender] >= (amountWithFee - 0.00000001)) {
            validTransactions.push_back(tx);
            spendingInThisBlock[sender] += amountWithFee;
            
            // Cálculo da taxa de 1%
            double fee = amountWithFee - (amountWithFee / 1.01);
            totalFees += fee;
        }
    }

    double subsidy = getBlockReward(chain.size());
    double totalReward = subsidy + totalFees;

    Transaction coinbase;
    coinbase.id = "coinbase_" + std::to_string(chain.size());
    coinbase.vout.push_back({minerAddress, totalReward});
    coinbase.signature = "coinbase";
    coinbase.publicKey = "SYSTEM_EMISSION";

    std::vector<Transaction> blockTxs;
    blockTxs.push_back(coinbase);
    blockTxs.insert(blockTxs.end(), validTransactions.begin(), validTransactions.end());

    Block newBlock(chain.size(), chain.back().hash, blockTxs);
    std::cout << "[MINER] Minerando bloco #" << newBlock.index << "..." << std::endl;
    newBlock.mine(difficulty);
    
    addBlock(newBlock);
    
    Storage::saveChain(*this, "data/blockchain.dat");
    utxoSet.saveToFile("data/utxo.dat"); 
    Storage::clearMempool("data/mempool.dat");

    std::cout << "🎯 Bloco #" << newBlock.index << " OK! Recompensa: " << totalReward << " MZ" << std::endl;
}

void Blockchain::addBlock(const Block& block) {
    chain.push_back(block);
    for(const auto& tx : block.transactions) {
        utxoSet.update(tx);
        // Apenas o subsídio entra para o supply global
        if(tx.signature == "coinbase") {
            totalSupply += getBlockReward(block.index);
        }
    }
}

bool Blockchain::verifyTransaction(const Transaction& tx) {
    if (tx.signature == "coinbase") return true;
    
    std::string senderAddress = "";
    for (const auto& out : tx.vout) {
        if (out.amount < 0) senderAddress = out.address;
    }

    // AJUSTE: Compatibilidade com endereço de 34 caracteres e SALT
    std::string cleanKey = tx.publicKey;
    std::string expectedAddress = "MZ" + Crypto::sha256_util(cleanKey + "SALT_MAZE_2026").substr(0, 32);
    
    return (senderAddress == expectedAddress);
}

void Blockchain::adjustDifficulty() {
    if (chain.size() < 11) return;
    const Block& lastBlock = chain.back();
    const Block& relayBlock = chain[chain.size() - 10];
    long timeTarget = 600; 
    long timeTaken = lastBlock.timestamp - relayBlock.timestamp;

    if (timeTaken < timeTarget / 2) difficulty++;
    else if (timeTaken > timeTarget * 2 && difficulty > 1) difficulty--;
}

double Blockchain::getBalance(std::string address) { return utxoSet.getBalance(address); }

void Blockchain::send(std::string from, std::string to, double amount, std::string seed) {
    double totalNeeded = amount * 1.01;
    if (getBalance(from) < totalNeeded) {
        std::cout << "❌ Saldo insuficiente!" << std::endl;
        return;
    }

    Transaction tx;
    // O ID da transação agora é calculado de forma única para evitar colisões
    tx.id = Crypto::sha256_util(from + to + std::to_string(amount) + std::to_string(std::time(0)));
    tx.vout.push_back({to, amount});
    tx.vout.push_back({from, totalNeeded * -1});
    tx.publicKey = seed;
    // Assinatura simplificada para este modelo de estudo
    tx.signature = "SIG_" + Crypto::sha256_util(seed).substr(0, 16);
    
    Storage::saveMempool(tx, "data/mempool.dat");
    std::cout << "✅ Transação enviada para a mempool!" << std::endl;
}

std::vector<Block> Blockchain::getChain() const { return chain; }
int Blockchain::getDifficulty() const { return difficulty; }
// Removi a redefinição do getTotalSupply daqui para evitar o erro de compilação.
