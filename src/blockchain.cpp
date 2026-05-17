// Copyright (c) 2026-present The MazeChain developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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
#include <mutex>   
#include <atomic>  
#include <filesystem> // Adicionado para gestão de pastas (C++17)

#include "blockchain.h"
#include "storage.h"
#include "crypto.h"
#include "mining_utils.h"
#include "checkpoints.h"   
#include "db_integrity.h" 
#include "difficulty.h"           
#include "mempool_limit.h" 
#include "mempool_audit.h" 
#include "crypto/sha256.h"

namespace fs = std::filesystem;

// Garante que o mutex global exigido pelo main.cpp e outros arquivos exista fisicamente
std::mutex g_blockchain_mutex;

// Caminhos atualizados para refletir a estrutura de blocos do Bitcoin
const std::string BLOCKS_DIR = "data/blocks";
const std::string DB_PATH = "data/blocks/blk00000.dat"; // Arquivo de dados principal
const std::string HASH_DB_PATH = "data/blockchain.hash"; 
const std::string UTXO_PATH = "data/utxo.dat";
const std::string MEMPOOL_PATH = "data/mempool.dat";

const std::string MAZE_RESERVE_FUND = "MZ_SYSTEM_RESERVE_FUND_NON_EXPENDABLE";
const double MITS_PER_COIN = 100000000.0; 
const double MIN_FEE_RELAY = 0.00001; 

Blockchain::Blockchain() {
    difficulty = 5; 
    totalSupply = 0;
    interruptMining.store(false); 

    // Cria a estrutura de pastas /data/blocks se não existir
    if (!fs::exists(BLOCKS_DIR)) {
        fs::create_directories(BLOCKS_DIR);
    }

    this->chain.clear();

    if (!DBIntegrity::VerifyIntegrity(DB_PATH, HASH_DB_PATH)) {
        std::cout << "⚠️ [ALERTA] Integridade do arquivo corrompida ou hash inexistente!" << std::endl;
    }

    if (this->chain.empty()) {
        Storage::loadChain(*this, DB_PATH);
    }

    if (this->chain.empty()) {
        std::vector<Transaction> genesisTxs;
        Transaction coinbase;

        coinbase.id = "coinbase_genesis_1714158289"; 
        coinbase.vout.push_back({"MZ_GENESIS_ADDRESS", 150.0}); 
        coinbase.signature = "coinbase";
        coinbase.publicKey = "MAZE_GENESIS_MESSAGE: The Times 21/Apr/2026 MazeChain reborn.";
        genesisTxs.push_back(coinbase);

        std::string zeroHash(64, '0');

        Block genesis(0, zeroHash, genesisTxs, "GENESIS_BLOCK", 0);
        genesis.timestamp = 1714158289; 

        genesis.hash = Blockchain::calculateProofOfWork(genesis.toHashString());

        std::cout << "\n[SISTEMA] HASH DO BLOCO GENESIS: " << genesis.hash << "\n" << std::endl;

        chain.push_back(genesis);

        // No genesis, salvamos o arquivo inicial
        Storage::saveChain(*this, DB_PATH);
        DBIntegrity::UpdateHash(DB_PATH, HASH_DB_PATH);
    } else {
        std::cout << "✅ Blockchain carregada do disco. Altura atual: " << chain.size() << std::endl;

        if (!isChainValid()) {
            std::cout << "🚨 [ERRO CRÍTICO] Falha na validação de consenso!" << std::endl;
        }
    }

    rebuildUTXO();
}

std::string Blockchain::calculateProofOfWork(const std::string& blockData) {
    uint8_t hash[32];

    CSHA256 sha;
    sha.Write((const uint8_t*)blockData.data(), blockData.size());
    sha.Finalize(hash);

    CSHA256 sha2;
    sha2.Write(hash, 32);
    sha2.Finalize(hash);

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
    int halving_count = height / 10000;

    if (height <= 10000) return 0.01; 
    if (height <= 20000) return 0.02; 
    if (height <= 30000) return 0.025; 

    if (halving_count >= 45 && halving_count < 64) return 0.05; 

    if (halving_count >= 64 || this->totalSupply >= 20000000.0) return 0.07; 

    return 0.03; 
}

double Blockchain::getBlockReward(int height) const {
    int interval = 10000; 
    int halving_count = height / interval;

    if (this->totalSupply >= 20000000.0 || halving_count >= 64) {
        double reserveBalance = getBalance(MAZE_RESERVE_FUND);
        if (reserveBalance > 0.00000100) {
            return reserveBalance * 0.0001; 
        }
        return 0.00000001; 
    }

    double reward = 150.0 * std::pow(0.92, halving_count);

    if (reward < 0.00000001) reward = 0.00000001;
    return reward;
}

void Blockchain::adjustDifficulty() {
    int nextHeight = static_cast<int>(chain.size());

    // Era Inicial (blocos 0-9999): dificuldade FIXA em 4
    if (nextHeight < 10000) {
        difficulty = 4;
        return;
    }

    // Era de Transição (blocos 10000-19999): dificuldade FIXA em 5
    if (nextHeight < 20000) {
        difficulty = 5;
        return;
    }

    // Era Dinâmica Avançada (blocos 20000+): dificuldade DINÂMICA, piso mínimo de 6
    int window = 100;
    if (nextHeight > window) {
        const Block& lastBlock = chain.back();
        const Block& startBlock = chain[chain.size() - window];
        difficulty = Difficulty::calculate_next_difficulty(
            nextHeight, difficulty, lastBlock.timestamp, startBlock.timestamp
        );
    } else {
        difficulty = 6;
    }
    if (difficulty < 6) difficulty = 6;
}

void Blockchain::rebuildUTXO() {
    std::cout << "🔄 Sincronizando saldos e calculando supply real..." << std::endl;

    std::lock_guard<std::mutex> lock(g_blockchain_mutex);

    utxoSet.utxoMap.clear(); 
    utxoSet.addressBalances.clear();
    totalSupply = 0; 

    for (const auto& block : chain) {
        double reward = getBlockReward(block.index);

        if (totalSupply + reward > 20000000.0) {
            reward = 20000000.0 - totalSupply;
        }
        if (reward < 0) reward = 0;

        totalSupply += reward;

        for (const auto& tx : block.transactions) {
            utxoSet.update(tx, block.index);
        }
    }
    std::cout << "✅ Sincronização concluída. Supply: " << totalSupply << " MZ" << std::endl;
}

bool Blockchain::mineBlock(std::string minerAddress) {
    int nextIndex = static_cast<int>(chain.size());
    std::string lastHash = chain.back().hash;

    adjustDifficulty();

    std::cout << "[MOTOR] ⛏️ Minerando Bloco #" << nextIndex << " (Dificuldade: " << difficulty << ")" << std::endl;

    if (minerAddress.length() < 30 || minerAddress.substr(0, 2) != "MZ") {
        std::cout << "❌ ERRO: Endereço inválido!" << std::endl;
        return false;
    }

    std::vector<Transaction> pending = Storage::loadMempool(MEMPOOL_PATH);
    std::vector<Transaction> validTransactions;
    double totalFees = 0;
    double feePercent = getDynamicFeePercentage(nextIndex);

    for (const auto& tx : pending) {
        if (!verifyTransaction(tx)) continue;
        validTransactions.push_back(tx);

        for(const auto& out : tx.vout) {
            if (out.amount < 0) { 
                totalFees += (std::abs(out.amount) * feePercent);
            }
        }
    }

    MinerIdentity id = prepare_miner_identity(
        minerAddress.c_str(), 
        nextIndex, 
        this->totalSupply, 
        getBalance(MAZE_RESERVE_FUND)
    );

    double subsidy = getBlockReward(nextIndex); 

    if (totalSupply + subsidy > 20000000.0) {
        subsidy = 20000000.0 - totalSupply;
    }

    Transaction coinbase;
    coinbase.id = "coinbase_h" + std::to_string(nextIndex) + "_" + std::to_string(std::time(nullptr));
    coinbase.vout.push_back({minerAddress, subsidy});

    if (totalFees > 0) {
        coinbase.vout.push_back({MAZE_RESERVE_FUND, totalFees});
    }

    coinbase.signature = "coinbase";
    coinbase.publicKey = "MAZE_BITCOIN_CORE_ENGINE_V1";

    std::vector<Transaction> blockTxs;
    blockTxs.push_back(coinbase);
    blockTxs.insert(blockTxs.end(), validTransactions.begin(), validTransactions.end());

    Block newBlock(nextIndex, lastHash, blockTxs, id.miner_address, id.extra_nonce);

    interruptMining.store(false);

    if (!newBlock.mineWithInterrupt(difficulty, interruptMining)) {
        std::cout << "⚠️ [MINER] Falha na mineração ou interrompido." << std::endl;
        return false;
    }

    std::lock_guard<std::mutex> lock_final(g_blockchain_mutex);

    if (nextIndex != (int)chain.size()) {
        std::cout << "❌ [RACE] Altura mudou durante mineração. Descartando." << std::endl;
        return false;
    }

    if (!Checkpoints::CheckBlock(newBlock.index, newBlock.hash)) {
        std::cout << "🚨 [CHECKPOINT BLOQUEOU] Hash inválido para esta altura!" << std::endl;
        return false;
    }

    chain.push_back(newBlock);

    // MELHORIA: Em vez de salvar a chain toda, anexamos apenas o novo bloco (Bitcoin Style)
    Storage::saveBlockToDisk(newBlock, DB_PATH); 
    DBIntegrity::UpdateHash(DB_PATH, HASH_DB_PATH);

    totalSupply += subsidy;

    for(const auto& tx : newBlock.transactions) {
        utxoSet.update(tx, newBlock.index);
    }

    utxoSet.saveToFile(UTXO_PATH);
    Storage::clearMempool(MEMPOOL_PATH);

    std::cout << "🎯 Bloco #" << newBlock.index << " Minerado! Subsídio: " << subsidy << " MZ | Taxas : " << totalFees << " MZ" << std::endl;

    return true;
}

void Blockchain::addBlock(const Block& block) {
    std::lock_guard<std::mutex> lock(g_blockchain_mutex);

    if (block.index < (int)chain.size()) return;

    if (block.index != (int)chain.size()) {
        std::cout << "⚠️ Index inválido: " << block.index << " esperado: " << chain.size() << std::endl;
        return;
    }

    adjustDifficulty();

    if (!Checkpoints::CheckBlock(block.index, block.hash)) {
        std::cout << "🚨 CHECKPOINT REJEITOU BLOCO EXTERNO " << block.index << std::endl;
        return;
    }

    chain.push_back(block);

    double reward = getBlockReward(block.index);
    if (totalSupply + reward > 20000000.0) reward = 20000000.0 - totalSupply;

    totalSupply += reward;

    for(const auto& tx : block.transactions) {
        utxoSet.update(tx, block.index);
    }

    std::cout << "✅ Bloco externo aceito. Nova altura: " << chain.size() << std::endl;
}

bool Blockchain::isChainValid() {
    if (chain.empty()) return false;

    for (size_t i = 1; i < chain.size(); i++) {
        const Block& currentBlock = chain[i];
        const Block& prevBlock = chain[i-1];

        if (currentBlock.prevHash != prevBlock.hash) {
            std::cout << "❌ Falha de encadeamento no bloco " << i << std::endl;
            return false;
        }

        if (currentBlock.hash != Blockchain::calculateProofOfWork(currentBlock.toHashString())) {
            std::cout << "❌ Hash inválido detectado no bloco " << i << std::endl;
            return false;
        }
    }
    return true;
}

void Blockchain::printStats() {
    std::cout << "\n==========================================" << std::endl;
    std::cout << "            ESTATÍSTICAS MAZECORE           " << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << " Altura da Rede     : " << chain.size() << std::endl;
    std::cout << " Circulante (Supply): " << std::fixed << std::setprecision(8) << totalSupply << " MZ" << std::endl;
    std::cout << " Fundo Reserva      : " << getBalance(MAZE_RESERVE_FUND) << " MZ" << std::endl;
    std::cout << " Dificuldade Atual  : " << difficulty << std::endl;
    std::cout << "==========================================\n" << std::endl;
}

double Blockchain::getBalance(std::string address) const { 
    return utxoSet.getBalance(address, static_cast<int>(chain.size())); 
}

bool Blockchain::verifyTransaction(const Transaction& tx) {
    if (tx.signature == "coinbase") return true;
    if (tx.vout.empty()) return false;

    std::string data_to_verify = tx.vout[0].address + std::to_string(tx.vout[0].amount);
    return Crypto::verify_signature(data_to_verify, tx.signature, tx.publicKey);
}

void Blockchain::send(std::string from, std::string to, double amount, std::string seed) {
    if (from == MAZE_RESERVE_FUND) throw std::runtime_error("Acesso negado ao Fundo de Reserva.");

    double currentFeePercent = getDynamicFeePercentage(static_cast<int>(chain.size()));
    double totalNeeded = amount + (amount * currentFeePercent);

    if (!MempoolAudit::IsTransactionAllowed(*this, from, totalNeeded, MEMPOOL_PATH)) {
        throw std::runtime_error("Saldo insuficiente ou transações pendentes no Mempool.");
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
    std::lock_guard<std::mutex> lock(g_blockchain_mutex);
    chain.clear(); 
    totalSupply = 0; 
    utxoSet.utxoMap.clear(); 
}

// Histórico Detalhado de Carteira para Carteira
void Blockchain::printAddressHistory(const std::string& targetAddress) {
    bool found = false;
    std::cout << std::fixed << std::setprecision(8);
    std::cout << "\n--- HISTÓRICO DE TRANSAÇÕES PARA: " << targetAddress << " ---\n" << std::endl;

    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        for (const auto& tx : it->transactions) {
            bool isSender = false;
            bool isReceiver = false;
            double amountSent = 0;
            double amountReceived = 0;
            std::string counterParty = "";

            for(const auto& out : tx.vout) {
                if (out.address == targetAddress) {
                    if (out.amount < 0) {
                        isSender = true;
                        amountSent = std::abs(out.amount);
                    } else {
                        isReceiver = true;
                        amountReceived = out.amount;
                    }
                } else {
                    if (out.address != MAZE_RESERVE_FUND) {
                        counterParty = out.address;
                    }
                }
            }

            if (isSender || isReceiver) {
                found = true;
                std::cout << "Bloco: #" << it->index;

                if (tx.signature == "coinbase") {
                    std::cout << " | [MINERAÇÃO] | Recompensa: +" << amountReceived << " MZ" << std::endl;
                } else if (isSender) {
                    std::cout << " | [ENVIADO]   | Valor: -" << amountSent << " MZ (Taxa incl.) -> " << (counterParty.empty() ? "Destino Externo" : counterParty) << std::endl;
                } else if (isReceiver) {
                    std::cout << " | [RECEBIDO]  | Valor: +" << amountReceived << " MZ" << std::endl;
                }

                std::cout << "ID Tx: " << tx.id << std::endl;
                std::cout << "----------------------------------------------------------------------" << std::endl;
            }
        }
    }
    if (!found) std::cout << "Nenhuma transação encontrada para este endereço." << std::endl;
}

// Getters exigidos pelo compilador
std::vector<Block> Blockchain::getChain() const { return chain; }
int Blockchain::getDifficulty() const { return difficulty; }
double Blockchain::getTotalSupply() const { return totalSupply; }
double Blockchain::getMaxSupply() const { return 20000000.0; }