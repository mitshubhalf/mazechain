#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <sys/uio.h>
#include <exception>
#include <cstring>
#include <chrono>
#include <iostream>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <map>
#include <algorithm>
#include <cstdint>
#include <vector>
#include <string>
#include <ctime>

#include "blockchain.h"
#include "storage.h"
#include "crypto.h"
#include "mining_utils.h"
#include "checkpoints.h"   
#include "db_integrity.h" 
#include "difficulty.h"       
#include "mempool_limit.h" 
#include "mempool_audit.h" 
#include "crypto/sha256.h" // INTEGRAÇÃO DO MOTOR BITCOIN CORE ATRAVÉS DO CABEÇALHO

// Caminhos de Dados
const std::string DB_PATH = "/home/runner/workspace/data/blockchain.dat";
const std::string HASH_DB_PATH = "/home/runner/workspace/data/blockchain.hash"; 
const std::string UTXO_PATH = "/home/runner/workspace/data/utxo.dat";
const std::string MEMPOOL_PATH = "/home/runner/workspace/data/mempool.dat";

const std::string MAZE_RESERVE_FUND = "MZ_SYSTEM_RESERVE_FUND_NON_EXPENDABLE";
const double MITS_PER_COIN = 100000000.0; 
const double MIN_FEE_RELAY = 0.00001; 

Blockchain::Blockchain() {
    difficulty = 4;
    totalSupply = 0;

    if (!DBIntegrity::VerifyIntegrity(DB_PATH, HASH_DB_PATH)) {
        std::cout << "⚠️ [ALERTA] Integridade do arquivo corrompida ou hash inexistente!" << std::endl;
    }

    Storage::loadChain(*this, DB_PATH);

    if (this->chain.empty()) {
        std::vector<Transaction> genesisTxs;
        Transaction coinbase;

        coinbase.id = "coinbase_genesis_1714158289"; 
        coinbase.vout.push_back({"MZ_GENESIS_ADDRESS", 400.0}); 
        coinbase.signature = "coinbase";
        coinbase.publicKey = "MAZE_GENESIS_MESSAGE: The Times 21/Apr/2026 MazeChain reborn.";
        genesisTxs.push_back(coinbase);

        std::string zeroHash(64, '0');

        Block genesis(0, zeroHash, genesisTxs, "GENESIS_BLOCK", 0);
        genesis.timestamp = 1714158289; 

        // Uso do motor CSHA256 (Bitcoin) para o cálculo do Genesis
        genesis.hash = Blockchain::calculateProofOfWork(genesis.toHashString()); 

        std::cout << "\n[SISTEMA] HASH DO BLOCO GENESIS: " << genesis.hash << "\n" << std::endl;

        chain.push_back(genesis);
        DBIntegrity::UpdateHash(DB_PATH, HASH_DB_PATH);
    } else {
        std::cout << "✅ Blockchain carregada do disco. Altura: " << chain.size() << std::endl;
        if (!isChainValid()) {
            std::cout << "🚨 [ERRO CRÍTICO] Falha na validação de consenso!" << std::endl;
        }
    }
    rebuildUTXO();
}

// --- MOTOR BITCOIN CORE INTEGRADO ---
std::string Blockchain::calculateProofOfWork(const std::string& blockData) {
    uint8_t hash[32];

    // SHA256 Round 1
    CSHA256 sha;
    sha.Write((const uint8_t*)blockData.data(), blockData.size());
    sha.Finalize(hash);

    // SHA256 Round 2 (Double SHA256 - Padrão Bitcoin)
    CSHA256 sha2;
    sha2.Write(hash, 32);
    sha2.Finalize(hash);

    // Conversão eficiente para Hexadecimal
    std::stringstream ss;
    for (int i = 0; i < 32; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

int Blockchain::getSafetyFloor() {
    int currentHeight = static_cast<int>(chain.size());
    int interval = 10000; 
    int lastManualCheckpoint = Checkpoints::GetLastCheckpointHeight();

    if (currentHeight < interval) return lastManualCheckpoint;

    int halvingFloor = (currentHeight / interval) * interval;
    return std::max(lastManualCheckpoint, halvingFloor);
}

double Blockchain::getDynamicFeePercentage(int height) {
    if (height <= 10000) return 0.01;
    if (height <= 20000) return 0.02;
    if (height <= 30000) return 0.025;

    int halving_count = height / 10000;
    if (halving_count >= 45 && this->totalSupply < 20000000.0) return 0.05; 
    if (this->totalSupply >= 20000000.0) return 0.07; 

    return 0.03;
}

double Blockchain::getBlockReward(int height) {
    if (this->totalSupply >= 20000000.0) {
        double reserveBalance = getBalance(MAZE_RESERVE_FUND);
        if (reserveBalance > 0.00000001) return reserveBalance * 0.0001;
        return 0.00000001;
    }

    int interval = 10000; 
    int halving_count = height / interval;
    if (halving_count >= 64) return 0.00000001;

    double reward = 400.0; 
    if (halving_count < 4) {
        for (int i = 0; i < halving_count; i++) reward *= 0.5;
    } 
    else if (halving_count < 20) {
        reward = 40.0; 
        for (int i = 4; i < halving_count; i++) reward *= 0.80;
    }
    else if (halving_count < 50) {
        reward = 1.407; 
        for (int i = 20; i <= halving_count; i++) reward *= 0.90;
    }
    else {
        reward = 0.060; 
        for (int i = 50; i < halving_count; i++) reward *= 0.98;
    }
    return (reward < 0.00000001) ? 0.00000001 : reward;
}

void Blockchain::adjustDifficulty() {
    int currentHeight = static_cast<int>(chain.size());
    if (currentHeight < 10) { difficulty = 4; return; }

    int window = (currentHeight > 20000) ? 100 : 10;
    if (static_cast<int>(chain.size()) >= window) {
        const Block& lastBlock = chain.back();
        const Block& startBlock = chain[chain.size() - window];

        difficulty = Difficulty::calculate_next_difficulty(
            currentHeight, difficulty, lastBlock.timestamp, startBlock.timestamp
        );
    }
}

void Blockchain::rebuildUTXO() {
    std::cout << "🔄 Sincronizando saldos..." << std::endl;
    utxoSet.utxoMap.clear(); 
    utxoSet.addressBalances.clear();
    totalSupply = 0; 

    for (const auto& block : chain) {
        if (block.transactions.empty() && block.index != 0) continue;

        if (!Checkpoints::CheckBlock(block.index, block.hash)) {
             if (block.index == 0 || block.index == Checkpoints::GetLastCheckpointHeight()) continue;
        }

        totalSupply += getBlockReward(block.index);
        for (const auto& tx : block.transactions) {
            utxoSet.update(tx, block.index);
        }
    }
    utxoSet.saveToFile(UTXO_PATH);
}

bool Blockchain::mineBlock(std::string minerAddress) {
    if (minerAddress.length() < 30 || minerAddress.substr(0, 2) != "MZ") {
        std::cout << "❌ ERRO: Endereço inválido!" << std::endl;
        return false;
    }

    adjustDifficulty();

    int currentHeight = static_cast<int>(chain.size());

    std::vector<Transaction> pending = Storage::loadMempool(MEMPOOL_PATH);
    std::vector<Transaction> validTransactions;
    double totalFees = 0;

    for (const auto& tx : pending) {
        if (!verifyTransaction(tx)) continue;
        validTransactions.push_back(tx);
        // Exemplo de cálculo de taxa (pode ser ajustado conforme sua regra)
        totalFees += 0.0001; 
    }

    MinerIdentity id = prepare_miner_identity(minerAddress.c_str(), currentHeight);
    double subsidy = getBlockReward(currentHeight); 

    Transaction coinbase;
    coinbase.id = "coinbase_h" + std::to_string(currentHeight) + "_" + std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
    coinbase.vout.push_back({minerAddress, subsidy});

    if (totalFees > 0) {
        coinbase.vout.push_back({MAZE_RESERVE_FUND, totalFees});
    }
    coinbase.signature = "coinbase";
    coinbase.publicKey = "MAZE_BITCOIN_CORE_ENGINE_V1";

    std::vector<Transaction> blockTxs;
    blockTxs.push_back(coinbase);
    blockTxs.insert(blockTxs.end(), validTransactions.begin(), validTransactions.end());

    Block newBlock(currentHeight, chain.back().hash, blockTxs, id.miner_address, id.extra_nonce);

    // O método mine() interno de Block deve utilizar o Blockchain::calculateProofOfWork
    newBlock.mine(difficulty); 

    addBlock(newBlock);

    Storage::saveChain(*this, DB_PATH);
    DBIntegrity::UpdateHash(DB_PATH, HASH_DB_PATH);
    utxoSet.saveToFile(UTXO_PATH); 
    Storage::clearMempool(MEMPOOL_PATH);

    std::cout << "🎯 Bloco #" << newBlock.index << " Minerado via CSHA256!" << std::endl;
    return true;
}

void Blockchain::addBlock(const Block& block) {
    int floor = getSafetyFloor();

    if (block.index < floor && block.index != 0) {
        if (!Checkpoints::CheckBlock(block.index, block.hash)) return;
    }

    if (!Checkpoints::CheckBlock(block.index, block.hash)) return;

    if (block.index >= (int)chain.size()) {
        chain.push_back(block);
        if (!block.transactions.empty()) {
            totalSupply += getBlockReward(block.index);
            for(const auto& tx : block.transactions) {
                utxoSet.update(tx, block.index);
            }
        }
    }
}

bool Blockchain::isChainValid() {
    if (chain.empty()) return false;

    for (size_t i = 1; i < chain.size(); i++) {
        if (chain[i].prevHash != chain[i-1].hash) return false;
        // Validação usando o novo motor SHA256 do Bitcoin
        if (chain[i].hash != Blockchain::calculateProofOfWork(chain[i].toHashString())) return false;
        if (!Checkpoints::CheckBlock(chain[i].index, chain[i].hash)) return false;
    }
    return true;
}

void Blockchain::printStats() {
    std::cout << "\n==========================================" << std::endl;
    std::cout << "           ESTATÍSTICAS MAZECORE           " << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << " Altura da Rede     : " << chain.size() << std::endl;
    std::cout << " Circulante (Supply): " << std::fixed << std::setprecision(8) << totalSupply << " MZ" << std::endl;
    std::cout << " Motor de Hash      : CSHA256 (Bitcoin Core Optimized)" << std::endl;
    std::cout << "==========================================\n" << std::endl;
}

double Blockchain::getBalance(std::string address) { 
    return utxoSet.getBalance(address, static_cast<int>(chain.size())); 
}

bool Blockchain::verifyTransaction(const Transaction& tx) {
    if (tx.signature == "coinbase") return true;
    if (tx.vout.size() < 2) return false;
    std::string data_to_verify = tx.vout[0].address + std::to_string(tx.vout[0].amount);
    return Crypto::verify_signature(data_to_verify, tx.signature, tx.publicKey);
}

void Blockchain::send(std::string from, std::string to, double amount, std::string seed) {
    if (from == MAZE_RESERVE_FUND) throw std::runtime_error("Acesso negado ao Fundo de Reserva.");

    double currentFeePercent = getDynamicFeePercentage(static_cast<int>(chain.size()));
    double totalNeeded = amount + (amount * currentFeePercent);

    if (!MempoolAudit::IsTransactionAllowed(*this, from, totalNeeded, MEMPOOL_PATH)) {
        throw std::runtime_error("Saldo insuficiente (Mempool pendente).");
    }

    Crypto::KeyPair keys = Crypto::generate_keys_from_seed(seed);
    Transaction tx;
    tx.id = Crypto::sha256d(from + to + std::to_string(amount) + std::to_string(std::time(nullptr)));
    tx.vout.push_back({to, amount}); 
    tx.vout.push_back({from, totalNeeded * -1}); 
    tx.publicKey = keys.public_key;
    tx.signature = Crypto::sign_data(to + std::to_string(amount), keys.private_key);

    Storage::saveMempool(tx, MEMPOOL_PATH);
}

void Blockchain::clearChain() { 
    chain.clear(); 
    totalSupply = 0; 
    utxoSet.utxoMap.clear(); 
}

std::vector<Block> Blockchain::getChain() const { return chain; }
int Blockchain::getDifficulty() const { return difficulty; }
double Blockchain::getTotalSupply() const { return totalSupply; }
double Blockchain::getMaxSupply() const { return 20000000.0; }