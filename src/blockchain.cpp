#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/crypto.h"
#include "../include/mining_utils.h"
#include <iostream>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <map>
#include <ctime>
#include <algorithm> 

// DEFINIÇÃO DOS CAMINHOS ABSOLUTOS PARA EVITAR "UNIVERSOS PARALELOS"
const std::string DB_PATH = "/home/runner/workspace/data/blockchain.dat";
const std::string UTXO_PATH = "/home/runner/workspace/data/utxo.dat";
const std::string MEMPOOL_PATH = "/home/runner/workspace/data/mempool.dat";

const double MITS_PER_COIN = 100000000.0; 
const double MIN_FEE_RELAY = 0.00001; 

Blockchain::Blockchain() {
    difficulty = 4;
    totalSupply = 0;

    // 1. CARREGAMENTO PASSIVO USANDO CAMINHO ABSOLUTO
    Storage::loadChain(*this, DB_PATH);

    if (this->chain.empty()) {
        std::vector<Transaction> genesisTxs;
        Block genesis(0, "0", genesisTxs, "GENESIS_BLOCK", 0);
        genesis.hash = genesis.calculateHash(); 
        chain.push_back(genesis);
        std::cout << "[SISTEMA] Novo Bloco Gênesis estabelecido." << std::endl;
    } else {
        std::cout << "✅ Blockchain carregada do disco. Altura: " << chain.size() << std::endl;
    }

    rebuildUTXO();
}

void Blockchain::rebuildUTXO() {
    std::cout << "🔄 Sincronizando saldos com a blockchain..." << std::endl;
    utxoSet.utxos.clear(); 
    totalSupply = 0;

    for (const auto& block : chain) {
        if (block.index > 0) {
            totalSupply += getBlockReward(block.index);
        }
        for (const auto& tx : block.transactions) {
            utxoSet.update(tx);
        }
    }

    utxoSet.saveToFile(UTXO_PATH);
    std::cout << "✅ Saldos sincronizados. Supply total: " << totalSupply << " MZ" << std::endl;
}

void Blockchain::clearChain() {
    chain.clear();
    totalSupply = 0;
    utxoSet.utxos.clear();
}

bool Blockchain::isChainValid() {
    if (chain.size() <= 1) return true;

    for (size_t i = 1; i < chain.size(); i++) {
        const Block& currentBlock = chain[i];
        const Block& prevBlock = chain[i-1];

        if (currentBlock.prevHash != prevBlock.hash) return false;

        int requiredDiff = (i > 1000) ? 5 : 1; 
        std::string target(requiredDiff, '0');
        if (currentBlock.hash.substr(0, requiredDiff) != target) return false;
    }
    return true;
}

void Blockchain::printStats() {
    std::cout << "\n==========================================" << std::endl;
    std::cout << "          ESTATÍSTICAS DA MAZECHAIN        " << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << " Altura da Rede     : " << chain.size() << " blocos" << std::endl;
    std::cout << " Dificuldade Atual  : " << difficulty << std::endl;

    if (totalSupply >= 10000000.0) {
        std::cout << " Tempo Alvo Atual   : 10 min" << std::endl;
    } else {
        std::cout << " Tempo Alvo Atual   : 1 min" << std::endl;
    }

    std::cout << " Circulante (Supply): " << std::fixed << std::setprecision(8) << totalSupply << " MZ" << std::endl;
    std::cout << " Supply Máximo      : " << std::fixed << std::setprecision(8) << getMaxSupply() << " MZ" << std::endl;
    std::cout << "==========================================\n" << std::endl;
}

double Blockchain::getBlockReward(int height) {
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

    adjustDifficulty();

    // MEMPOOL COM CAMINHO ABSOLUTO
    std::vector<Transaction> pending = Storage::loadMempool(MEMPOOL_PATH);
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
        }
    }

    int currentHeight = static_cast<int>(chain.size());
    MinerIdentity id = prepare_miner_identity(minerAddress.c_str(), currentHeight);

    double subsidy = getBlockReward(currentHeight); 
    double totalReward = subsidy + totalFees;

    Transaction coinbase;
    coinbase.id = "coinbase_h" + std::to_string(currentHeight) + "_" + std::to_string(std::time(nullptr));
    coinbase.vout.push_back({minerAddress, totalReward});
    coinbase.signature = "coinbase";
    coinbase.publicKey = "MAZE_EMISSION_SYSTEM_2026";

    std::vector<Transaction> blockTxs;
    blockTxs.push_back(coinbase);
    blockTxs.insert(blockTxs.end(), validTransactions.begin(), validTransactions.end());

    Block newBlock(currentHeight, chain.back().hash, blockTxs, id.miner_address, id.extra_nonce);

    std::cout << "⛏️ Iniciando busca de Hash (Dificuldade: " << difficulty << ")..." << std::endl;
    newBlock.mine(difficulty);

    addBlock(newBlock);

    // SALVAMENTO COM CAMINHOS ABSOLUTOS
    Storage::saveChain(*this, DB_PATH);
    utxoSet.saveToFile(UTXO_PATH); 
    Storage::clearMempool(MEMPOOL_PATH);

    std::cout << "🎯 Bloco #" << newBlock.index << " OK! Txs: " << blockTxs.size() << " | Recompensa: " << subsidy << " MZ" << std::endl;
}

void Blockchain::addBlock(const Block& block) {
    if (block.index >= (int)chain.size()) {
        chain.push_back(block);
        if (block.index > 0) {
            totalSupply += getBlockReward(block.index);
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
    int currentHeight = static_cast<int>(chain.size());

    if (currentHeight <= 1000) {
        difficulty = 4;
    } else {
        if (difficulty < 5) {
            difficulty = 5;
            std::cout << "🛡️ [POLÍTICA] Piso de dificuldade 5 ativado (Bloco 1001+)." << std::endl;
        }
    }

    if (currentHeight < 11) return;

    const Block& lastBlock = chain.back();
    const Block& relayBlock = chain[chain.size() - 10];

    long timeTargetSeconds = (totalSupply >= 10000000.0) ? 600 : 60;
    long timeTaken = static_cast<long>(lastBlock.timestamp - relayBlock.timestamp);
    long targetFor10Blocks = timeTargetSeconds * 10;

    if (timeTaken < (targetFor10Blocks / 2)) {
        difficulty++;
    } else if (timeTaken > (targetFor10Blocks * 2)) {
        difficulty--;
    }

    if (currentHeight <= 1000 && difficulty > 4) difficulty = 4;
    if (currentHeight > 1000 && difficulty < 5) difficulty = 5;
}

double Blockchain::getBalance(std::string address) { 
    return utxoSet.getBalance(address); 
}

void Blockchain::send(std::string from, std::string to, double amount, std::string seed) {
    double fee = amount * 0.01;
    double totalNeeded = amount + fee;

    if (amount <= 0) throw std::runtime_error("Valor invalido.");
    if (getBalance(from) < totalNeeded) throw std::runtime_error("Saldo insuficiente.");

    Transaction tx;
    tx.id = Crypto::sha256_util(from + to + std::to_string(amount) + std::to_string(std::time(nullptr)));
    tx.vout.push_back({to, amount});
    tx.vout.push_back({from, totalNeeded * -1}); 
    tx.publicKey = seed;
    tx.signature = "SIG_" + Crypto::sha256_util(seed + tx.id).substr(0, 24);

    Storage::saveMempool(tx, MEMPOOL_PATH);
}

std::vector<Block> Blockchain::getChain() const { return chain; }
int Blockchain::getDifficulty() const { return difficulty; }
double Blockchain::getTotalSupply() const { return totalSupply; }
double Blockchain::getMaxSupply() const { return 20000000.0; }