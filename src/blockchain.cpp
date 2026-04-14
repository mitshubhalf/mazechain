#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/crypto.h"
#include <iostream>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <map>

// Constantes da Economia MazeChain
const double MAX_SUPPLY = 20000000.0; // 20 Milhões de MZ
const double MITS_PER_COIN = 100000000.0; // 100 Milhões de Mits = 1 MZ

Blockchain::Blockchain() {
    difficulty = 4;
    totalSupply = 0;
    
    // Tenta carregar o estado anterior
    utxoSet.loadFromFile("data/utxo.dat");

    if (chain.empty()) {
        // Bloco Gênesis: No Bitcoin, o Bloco 0 não era gastável. 
        // Aqui, criamos ele vazio. O saldo real virá no Bloco #1 minerado por VOCÊ.
        std::vector<Transaction> genesisTxs;
        Block genesis(0, "0", genesisTxs);
        genesis.hash = genesis.calculateHash(); // Gênesis não precisa minerar se estiver vazio
        
        chain.push_back(genesis);
        std::cout << "[SISTEMA] Bloco Gênesis estabelecido. Aguardando primeira mineração manual." << std::endl;
    }
}

double Blockchain::getBlockReward(int height) {
    // Se já atingiu 20M, a recompensa de 'novas moedas' é ZERO. 
    // O minerador passará a viver apenas de taxas.
    if (totalSupply >= MAX_SUPPLY) return 0.0;

    double reward = 1000.0;
    
    // Regra: 0-1000 (1000), 1001-2000 (500), 2001-4000 (250)...
    if (height > 0 && height <= 1000) {
        reward = 1000.0;
    } else if (height > 1000 && height <= 2000) {
        reward = 500.0;
    } else if (height > 2000 && height <= 4000) {
        reward = 250.0;
    } else if (height > 4000) {
        // Segue halving contínuo a cada 2000 blocos após o marco de 4000
        int halvings = (height - 4000) / 2000 + 3; 
        reward = 1000.0 / std::pow(2, halvings);
    }

    // Garante que não ultrapasse o limite máximo no último bloco
    if (totalSupply + reward > MAX_SUPPLY) {
        reward = MAX_SUPPLY - totalSupply;
    }

    return (reward < 0.00000001) ? 0.0 : reward;
}

void Blockchain::mineBlock(std::string minerAddress) {
    if (minerAddress.substr(0, 2) != "MZ") {
        std::cout << "❌ Endereço de minerador inválido!" << std::endl;
        return;
    }

    adjustDifficulty();

    // 1. Carregar Mempool
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
        if (currentBalance - spendingInThisBlock[sender] >= (amountWithFee - 0.00000001)) {
            validTransactions.push_back(tx);
            spendingInThisBlock[sender] += amountWithFee;
            
            // A taxa de 1% já está inclusa no débito. Aqui extraímos ela para o minerador.
            // Valor Bruto = Líquido * 1.01 -> Taxa = Valor Bruto - (Valor Bruto / 1.01)
            double fee = amountWithFee - (amountWithFee / 1.01);
            totalFees += fee;
        }
    }

    // 2. Subsídio + Taxas (Sua lógica de economia infinita)
    double subsidy = getBlockReward(chain.size());
    
    // Se o subsídio acabou (chegou em 20M), totalReward será apenas as Taxas.
    // Isso mantém os mineradores ativos para sempre.
    double totalReward = subsidy + totalFees;

    Transaction coinbase;
    coinbase.id = "coinbase_" + std::to_string(chain.size()) + "_" + std::to_string(std::time(0));
    coinbase.vout.push_back({minerAddress, totalReward});
    coinbase.signature = "coinbase";
    coinbase.publicKey = "SYSTEM_EMISSION";

    // 3. Montar Bloco
    std::vector<Transaction> blockTxs;
    blockTxs.push_back(coinbase);
    blockTxs.insert(blockTxs.end(), validTransactions.begin(), validTransactions.end());

    // 4. Proof of Work
    Block newBlock(chain.size(), chain.back().hash, blockTxs);
    std::cout << "[MINER] Minerando bloco #" << newBlock.index << " (Dificuldade: " << difficulty << ")..." << std::endl;
    newBlock.mine(difficulty);
    
    // 5. Adicionar à rede e atualizar UTXOs
    addBlock(newBlock);
    
    // 6. Persistência
    Storage::saveChain(*this, "data/blockchain.dat");
    utxoSet.saveToFile("data/utxo.dat"); 
    Storage::clearMempool("data/mempool.dat");

    std::cout << "🎯 Bloco #" << newBlock.index << " minerado! Recompensa: " 
              << std::fixed << std::setprecision(8) << totalReward << " MZ (Subsídio: " 
              << subsidy << " + Taxas: " << totalFees << ")" << std::endl;
}

// --- Restante das funções de validação e utilidade ---

void Blockchain::addBlock(const Block& block) {
    chain.push_back(block);
    for(const auto& tx : block.transactions) {
        utxoSet.update(tx);
        // Apenas o subsídio novo aumenta o totalSupply. 
        // As taxas já existiam, então não aumentam o supply global.
        if(tx.signature == "coinbase") {
            double subsidy = getBlockReward(block.index);
            totalSupply += subsidy;
        }
    }
}

bool Blockchain::verifyTransaction(const Transaction& tx) {
    if (tx.signature == "coinbase") return true;
    if (tx.signature.empty() || tx.publicKey.empty()) return false;

    std::string senderAddress = "";
    for (const auto& out : tx.vout) {
        if (out.amount < 0) senderAddress = out.address;
    }

    // Validação básica de chave pública -> endereço
    std::string cleanKey = tx.publicKey;
    cleanKey.erase(0, cleanKey.find_first_not_of(" \t\r\n"));
    cleanKey.erase(cleanKey.find_last_not_of(" \t\r\n") + 1);
    std::string expectedAddress = "MZ" + Crypto::sha256_util(cleanKey).substr(0, 20);
    
    return (senderAddress == expectedAddress);
}

void Blockchain::adjustDifficulty() {
    if (chain.size() < 11) return;
    
    const Block& lastBlock = chain.back();
    const Block& relayBlock = chain[chain.size() - 10];

    long timeTarget = 600; // Alvo de 1 minuto para 10 blocos (ajuste conforme necessário)
    long timeTaken = lastBlock.timestamp - relayBlock.timestamp;

    if (timeTaken < timeTarget / 2) difficulty++;
    else if (timeTaken > timeTarget * 2 && difficulty > 1) difficulty--;
}

double Blockchain::getBalance(std::string address) { return utxoSet.getBalance(address); }
void Blockchain::send(std::string from, std::string to, double amount, std::string seed) {
    double totalNeeded = amount * 1.01;
    if (getBalance(from) < totalNeeded) return;

    Transaction tx;
    tx.id = Crypto::sha256_util(from + to + std::to_string(amount) + std::to_string(std::time(0)));
    tx.vout.push_back({to, amount});
    tx.vout.push_back({from, totalNeeded * -1});
    tx.publicKey = seed;
    tx.signature = "SIG_" + Crypto::sha256_util(seed).substr(0, 16);
    
    Storage::saveMempool(tx, "data/mempool.dat");
}

std::vector<Block> Blockchain::getChain() const { return chain; }
int Blockchain::getDifficulty() const { return difficulty; }
double Blockchain::getTotalSupply() const { return totalSupply; }
