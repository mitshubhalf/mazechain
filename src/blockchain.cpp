#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/crypto.h"
#include <iostream>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <map>
#include <ctime>
#include <algorithm> 

// Constantes da Economia MazeChain
const double MITS_PER_COIN = 100000000.0; 
const double MIN_FEE_RELAY = 0.00001; 

Blockchain::Blockchain() {
    difficulty = 4;
    totalSupply = 0;
    
    // 1. Tenta carregar a Blockchain primeiro (ela é a base da verdade)
    std::vector<Block> loadedChain = Storage::loadChain("data/blockchain.dat");
    
    if (loadedChain.empty()) {
        std::vector<Transaction> genesisTxs;
        Block genesis(0, "0", genesisTxs);
        genesis.hash = genesis.calculateHash(); 
        chain.push_back(genesis);
        std::cout << "[SISTEMA] Bloco Gênesis estabelecido com sucesso." << std::endl;
    } else {
        chain.push_back(loadedChain[0]); 
        
        for (size_t i = 1; i < loadedChain.size(); i++) {
            const Block& current = loadedChain[i];
            const Block& prev = chain.back();

            bool hashOk = (current.hash == current.calculateHash());
            bool linkOk = (current.prevHash == prev.hash);
            std::string target(difficulty, '0');
            bool diffOk = (current.hash.substr(0, difficulty) == target);

            if (hashOk && linkOk && diffOk) {
                chain.push_back(current);
            } else {
                std::cout << "⚠️ [ALERTA] Bloco #" << i << " inválido detectado no disco!" << std::endl;
                std::cout << "⚠️ A chain será truncada no bloco #" << (i-1) << " para evitar corrupção." << std::endl;
                break; 
            }
        }
        std::cout << "✅ Blockchain carregada com sucesso. Altura: " << chain.size() << std::endl;
    }

    // 2 & 3. RECONSTRUÇÃO DO ESTADO (UTXO e Supply)
    // Em vez de confiar no utxo.dat, nós o reconstruímos a partir da chain validada
    rebuildUTXO();
}

void Blockchain::rebuildUTXO() {
    std::cout << "🔄 Sincronizando saldos com a blockchain..." << std::endl;
    utxoSet.utxos.clear(); // Limpa o estado antigo
    totalSupply = 0;       // Zera o supply para recontar

    for (const auto& block : chain) {
        // Recontar o supply (apenas o subsídio, taxas não geram novas moedas)
        if (block.index > 0) {
            totalSupply += getBlockReward(block.index);
        }

        // Reprocessa cada transação para reconstruir o mapa de saldos exato
        for (const auto& tx : block.transactions) {
            utxoSet.update(tx);
        }
    }
    
    // Salva o estado limpo e sincronizado
    utxoSet.saveToFile("data/utxo.dat");
    std::cout << "✅ Saldos sincronizados. Supply total: " << totalSupply << " MZ" << std::endl;
}

void Blockchain::clearChain() {
    chain.clear();
    totalSupply = 0;
    utxoSet.utxos.clear();
}

bool Blockchain::isChainValid() {
    for (size_t i = 1; i < chain.size(); i++) {
        const Block& currentBlock = chain[i];
        const Block& prevBlock = chain[i-1];

        if (currentBlock.hash != currentBlock.calculateHash()) return false;
        if (currentBlock.prevHash != prevBlock.hash) return false;

        std::string target(difficulty, '0');
        if (currentBlock.hash.substr(0, difficulty) != target) return false;
    }
    return true;
}

void Blockchain::printStats() {
    std::cout << "\n==========================================" << std::endl;
    std::cout << "          ESTATÍSTICAS DA MAZECHAIN        " << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << " Altura da Rede     : " << chain.size() << " blocos" << std::endl;
    std::cout << " Dificuldade Atual  : " << difficulty << std::endl;
    std::cout << " Circulante (Supply): " << std::fixed << std::setprecision(8) << totalSupply << " MZ" << std::endl;
    std::cout << " Supply Máximo      : " << std::fixed << std::setprecision(8) << getMaxSupply() << " MZ" << std::endl;
    std::cout << "==========================================\n" << std::endl;
}

double Blockchain::getBlockReward(int height) {
    // Calculamos baseado na altura informada, não no totalSupply atual
    // para permitir a reconstrução precisa.
    double reward = 2000.0;
    if (height <= 1000) reward = 2000.0;
    else if (height <= 2000) reward = 1000.0;
    else if (height <= 4000) reward = 500.0;
    else {
        int currentHalvingBlockLimit = 4000;
        reward = 500.0;
        while (height > currentHalvingBlockLimit) {
            currentHalvingBlockLimit *= 2;
            reward /= 2.0;
            if (reward < 0.00000001) { reward = 0.0; break; }
        }
    }
    return reward;
}

void Blockchain::mineBlock(std::string minerAddress) {
    if (minerAddress.length() < 30 || minerAddress.substr(0, 2) != "MZ") {
        std::cout << "❌ ERRO: Endereço de minerador inválido!" << std::endl;
        return;
    }

    if (!isChainValid()) {
        std::cout << "❌ ERRO: Chain em memória inválida. Abortando mineração." << std::endl;
        return;
    }

    adjustDifficulty();

    std::vector<Transaction> pending = Storage::loadMempool("data/mempool.dat");
    std::vector<Transaction> validTransactions;
    double totalFees = 0;
    std::map<std::string, double> spendingInThisBlock;

    for (const auto& tx : pending) {
        if (tx.publicKey.empty()) continue; 

        if (!verifyTransaction(tx)) {
            std::cout << "⚠️ Tx ignorada: Falha na assinatura/verificação." << std::endl;
            continue;
        }
        
        std::string sender = "";
        double amountWithFee = 0;
        double netValue = 0;
        
        for (const auto& out : tx.vout) {
            if (out.amount < 0) { 
                sender = out.address; 
                amountWithFee = std::abs(out.amount); 
            } else {
                netValue += out.amount;
            }
        }

        if (sender.empty()) continue;

        double currentBalance = getBalance(sender);
        
        if (currentBalance - spendingInThisBlock[sender] >= amountWithFee) {
            validTransactions.push_back(tx);
            spendingInThisBlock[sender] += amountWithFee;

            double fee = amountWithFee - netValue;
            totalFees += std::max(0.0, fee);
        } else {
            std::cout << "⚠️ Tx ignorada: Saldo insuficiente para " << sender << std::endl;
        }
    }

    double subsidy = getBlockReward(static_cast<int>(chain.size()));
    double totalReward = subsidy + totalFees;

    Transaction coinbase;
    coinbase.id = "coinbase_h" + std::to_string(chain.size()) + "_" + std::to_string(std::time(nullptr));
    coinbase.vout.push_back({minerAddress, totalReward});
    coinbase.signature = "coinbase";
    coinbase.publicKey = "MAZE_EMISSION_SYSTEM_2026";

    std::vector<Transaction> blockTxs;
    blockTxs.push_back(coinbase);
    blockTxs.insert(blockTxs.end(), validTransactions.begin(), validTransactions.end());

    Block newBlock(static_cast<int>(chain.size()), chain.back().hash, blockTxs);
    
    std::cout << "⛏️ Iniciando busca de Hash (Dificuldade: " << difficulty << ")..." << std::endl;
    newBlock.mine(difficulty);
    
    addBlock(newBlock);
    
    // IMPORTANTE: Storage::saveChain deve usar std::ios::trunc para limpar o lixo antigo do arquivo
    Storage::saveChain(*this, "data/blockchain.dat");
    utxoSet.saveToFile("data/utxo.dat"); 
    Storage::clearMempool("data/mempool.dat");

    std::cout << "🎯 Bloco #" << newBlock.index << " OK! Txs: " << blockTxs.size() << " | Recompensa: " << subsidy << " MZ + " << totalFees << " Fees" << std::endl;
}

void Blockchain::addBlock(const Block& block) {
    if (block.index >= chain.size()) {
        chain.push_back(block);

        if (block.index > 0) {
            double reward = getBlockReward(block.index);
            totalSupply += reward;
        }

        for(const auto& tx : block.transactions) {
            utxoSet.update(tx);
        }
    }
}

bool Blockchain::verifyTransaction(const Transaction& tx) {
    if (tx.signature == "coinbase") return true;
    if (tx.publicKey.empty()) return false;

    std::string senderAddress = "";
    for (const auto& out : tx.vout) {
        if (out.amount < 0) {
            senderAddress = out.address;
            break;
        }
    }

    if (senderAddress.empty()) return false;

    std::string h1 = Crypto::sha256_util(tx.publicKey);
    std::string expectedAddress = "MZ" + Crypto::sha256_util(h1 + "SALT_MAZE_2026_PRODUCTION").substr(0, 32);
    
    return (senderAddress == expectedAddress);
}

void Blockchain::adjustDifficulty() {
    if (chain.size() < 11) return;

    const Block& lastBlock = chain.back();
    const Block& relayBlock = chain[chain.size() - 10];
    
    long timeTarget = 600; 
    long timeTaken = static_cast<long>(lastBlock.timestamp - relayBlock.timestamp);

    if (timeTaken < (timeTarget / 2)) {
        difficulty++;
    } else if (timeTaken > (timeTarget * 2) && difficulty > 1) {
        difficulty--;
    }
}

double Blockchain::getBalance(std::string address) { 
    return utxoSet.getBalance(address); 
}

void Blockchain::send(std::string from, std::string to, double amount, std::string seed) {
    double fee = amount * 0.01;
    double totalNeeded = amount + fee;

    if (amount <= 0) throw std::runtime_error("Valor invalido.");
    if (getBalance(from) < totalNeeded) throw std::runtime_error("Saldo insuficiente.");
    if (seed.empty()) throw std::runtime_error("Seed invalida.");

    Transaction tx;
    tx.id = Crypto::sha256_util(from + to + std::to_string(amount) + std::to_string(std::time(nullptr)));
    tx.vout.push_back({to, amount});
    tx.vout.push_back({from, totalNeeded * -1}); 
    tx.publicKey = seed;
    tx.signature = "SIG_" + Crypto::sha256_util(seed + tx.id).substr(0, 24);
    
    Storage::saveMempool(tx, "data/mempool.dat");
}

std::vector<Block> Blockchain::getChain() const { return chain; }
int Blockchain::getDifficulty() const { return difficulty; }
double Blockchain::getTotalSupply() const { return totalSupply; }
double Blockchain::getMaxSupply() const { return 20000000.0; }
