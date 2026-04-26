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

    // CHAMADA: DBIntegrity para verificar se o banco de dados foi violado fisicamente
    if (!DBIntegrity::VerifyIntegrity(DB_PATH, HASH_DB_PATH)) {
        std::cout << "⚠️ [ALERTA] Integridade do arquivo corrompida ou hash inexistente!" << std::endl;
        std::cout << "O sistema tentará carregar e validar os blocos via lógica de Checkpoint." << std::endl;
    }

    Storage::loadChain(*this, DB_PATH);

    if (this->chain.empty()) {
        std::vector<Transaction> genesisTxs;
        Transaction coinbase;
        coinbase.id = "coinbase_genesis_" + std::to_string(std::time(nullptr));
        coinbase.vout.push_back({"MZ_GENESIS_ADDRESS", 400.0}); 
        coinbase.signature = "coinbase";
        coinbase.publicKey = "MAZE_GENESIS_MESSAGE: The Times 21/Apr/2026 MazeChain reborn.";
        genesisTxs.push_back(coinbase);

        std::string zeroHash(64, '0');
        Block genesis(0, zeroHash, genesisTxs, "GENESIS_BLOCK", 0);
        genesis.hash = genesis.calculateHash(); 
        chain.push_back(genesis);

        std::cout << "[SISTEMA] Novo Bloco Gênesis estabelecido (400 MZ)." << std::endl;

        // CHAMADA: Atualiza integridade para o novo arquivo criado
        DBIntegrity::UpdateHash(DB_PATH, HASH_DB_PATH);
    } else {
        std::cout << "✅ Blockchain carregada do disco. Altura: " << chain.size() << std::endl;
        if (!isChainValid()) {
            std::cout << "🚨 [ERRO CRÍTICO] A blockchain carregada falhou na validação de consenso!" << std::endl;
        }
    }
    rebuildUTXO();
}

int Blockchain::getSafetyFloor() {
    int currentHeight = static_cast<int>(chain.size());
    int interval = 10000; 
    int halving_count = currentHeight / interval;

    // CHAMADA: Busca altura protegida no arquivo de Checkpoints
    int lastManualCheckpoint = Checkpoints::GetLastCheckpointHeight();

    if (halving_count == 0) return lastManualCheckpoint; 
    int halvingFloor = halving_count * interval;

    // O piso de segurança impede rollbacks abaixo do último halving ou checkpoint fixado
    return std::max(lastManualCheckpoint, halvingFloor);
}

double Blockchain::getDynamicFeePercentage(int height) {
    if (height <= 10000) return 0.01;
    if (height <= 20000) return 0.02;
    if (height <= 30000) return 0.025;

    int halving_count = height / 10000;

    // Regra de escassez: Quando o supply chega perto do limite (20M), taxas sobem para manter rede
    if (halving_count >= 45 && this->totalSupply < 20000000.0) return 0.05; 
    if (this->totalSupply >= 20000000.0) return 0.07; 

    return 0.03;
}

double Blockchain::getBlockReward(int height) {
    // REGRA 20M: Se o supply atingiu o limite, recompensa vira deflacionária vinda da reserva
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
    if (currentHeight < 10) {
        difficulty = 4;
        return;
    }

    // CHAMADA: Usa a lógica do arquivo difficulty.cpp para ajuste preciso (DGW ou Linear)
    int window = (currentHeight > 20000) ? 100 : 10;
    if (static_cast<int>(chain.size()) >= window) {
        const Block& lastBlock = chain.back();
        const Block& startBlock = chain[chain.size() - window];

        difficulty = Difficulty::calculate_next_difficulty(
            currentHeight,
            difficulty,
            lastBlock.timestamp,
            startBlock.timestamp
        );
    }
}

void Blockchain::rebuildUTXO() {
    std::cout << "🔄 Sincronizando saldos com a blockchain..." << std::endl;
    utxoSet.utxoMap.clear(); 
    utxoSet.addressBalances.clear();
    totalSupply = 0; 
    for (const auto& block : chain) {
        // CHAMADA: Valida cada bloco contra a lista de Checkpoints Hardcoded
        if (!Checkpoints::CheckBlock(block.index, block.hash)) {
             std::cout << "🚨 [ERRO] Falha de Checkpoint detectada no rebuild para bloco #" << block.index << std::endl;
             continue; 
        }
        totalSupply += getBlockReward(block.index);
        for (const auto& tx : block.transactions) {
            utxoSet.update(tx, block.index);
        }
    }
    utxoSet.saveToFile(UTXO_PATH);
    std::cout << "✅ Saldos sincronizados. Supply total: " << std::fixed << std::setprecision(8) << totalSupply << " MZ" << std::endl;
}

void Blockchain::mineBlock(std::string minerAddress) {
    if (minerAddress.length() < 30 || minerAddress.substr(0, 2) != "MZ") {
        std::cout << "❌ ERRO: Endereço de minerador inválido!" << std::endl;
        return;
    }

    adjustDifficulty();

    int currentHeight = static_cast<int>(chain.size());
    double currentFeePercent = getDynamicFeePercentage(currentHeight);

    std::vector<Transaction> pending = Storage::loadMempool(MEMPOOL_PATH);

    struct PrioritizedTx { Transaction tx; double feePaid; double totalValueWithFee; std::string sender; };
    std::vector<PrioritizedTx> prioritizedPool;

    for (const auto& tx : pending) {
        if (tx.publicKey.empty() || !verifyTransaction(tx)) continue;

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

        if (sender.empty() || sender == MAZE_RESERVE_FUND) continue;

        double actualFee = amountWithFee - netValue;
        double minRequiredFee = netValue * currentFeePercent;

        if (actualFee >= minRequiredFee && getBalance(sender) >= amountWithFee) {
            prioritizedPool.push_back({tx, actualFee, amountWithFee, sender});
        }
    }

    std::sort(prioritizedPool.begin(), prioritizedPool.end(), [](const PrioritizedTx& a, const PrioritizedTx& b) {
        return a.feePaid > b.feePaid;
    });

    std::vector<Transaction> validTransactions;
    double totalFees = 0;
    std::map<std::string, double> spendingInThisBlock;

    for (size_t i = 0; i < prioritizedPool.size() && i < 50; ++i) {
        const auto& ptx = prioritizedPool[i];

        if (getBalance(ptx.sender) - spendingInThisBlock[ptx.sender] >= ptx.totalValueWithFee) {
            validTransactions.push_back(ptx.tx);
            spendingInThisBlock[ptx.sender] += ptx.totalValueWithFee;
            totalFees += ptx.feePaid;
        }
    }

    // CHAMADA: MiningUtils para preparar os campos de segurança do bloco (ExtraNonce)
    MinerIdentity id = prepare_miner_identity(minerAddress.c_str(), currentHeight);
    double subsidy = getBlockReward(currentHeight); 

    Transaction coinbase;
    coinbase.id = "coinbase_h" + std::to_string(currentHeight) + "_" + std::to_string(std::time(nullptr));
    coinbase.vout.push_back({minerAddress, subsidy});

    if (totalFees > 0) {
        coinbase.vout.push_back({MAZE_RESERVE_FUND, totalFees});
    }

    coinbase.signature = "coinbase";
    coinbase.publicKey = "MAZE_DYNAMIC_FEE_LOG_V4";

    std::vector<Transaction> blockTxs;
    blockTxs.push_back(coinbase);
    blockTxs.insert(blockTxs.end(), validTransactions.begin(), validTransactions.end());

    Block newBlock(currentHeight, chain.back().hash, blockTxs, id.miner_address, id.extra_nonce);
    std::cout << "⛏️ Buscando Hash (Dificuldade: " << difficulty << " | Era: " << (currentFeePercent * 100) << "% Taxa)..." << std::endl;
    newBlock.mine(difficulty);

    addBlock(newBlock);

    Storage::saveChain(*this, DB_PATH);

    // CHAMADA: Atualiza hash de integridade após novo bloco salvo
    DBIntegrity::UpdateHash(DB_PATH, HASH_DB_PATH);

    utxoSet.saveToFile(UTXO_PATH); 
    Storage::clearMempool(MEMPOOL_PATH);

    std::cout << "🎯 Bloco #" << newBlock.index << " OK! Coletado em taxas: " << totalFees << " MZ" << std::endl;
}

void Blockchain::addBlock(const Block& block) {
    int floor = getSafetyFloor();
    if (block.index < floor && !chain.empty()) {
        std::cout << "🚨 [SEGURANÇA] Violação de piso! Bloco #" << block.index << " rejeitado." << std::endl;
        return;
    }

    // CHAMADA: Validação de Checkpoint obrigatória na inserção
    if (!Checkpoints::CheckBlock(block.index, block.hash)) {
        std::cout << "🚨 [SEGURANÇA] Hash inválido para Checkpoint no bloco #" << block.index << std::endl;
        return;
    }

    if (block.index >= (int)chain.size()) {
        chain.push_back(block);
        totalSupply += getBlockReward(block.index);
        for(const auto& tx : block.transactions) {
            utxoSet.update(tx, block.index);
        }
    }
}

bool Blockchain::isChainValid() {
    int floor = getSafetyFloor();
    if (chain.empty() || (int)chain.size() < floor) return false;
    for (size_t i = 1; i < chain.size(); i++) {
        if (chain[i].prevHash != chain[i-1].hash) return false;
        if (chain[i].hash != chain[i].calculateHash()) return false;

        // CHAMADA: Validação por checkpoints na varredura completa
        if (!Checkpoints::CheckBlock(chain[i].index, chain[i].hash)) return false;
    }
    return true;
}

void Blockchain::printStats() {
    std::cout << "\n==========================================" << std::endl;
    std::cout << "            ESTATÍSTICAS DA MAZECHAIN        " << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << " Altura da Rede     : " << chain.size() << " blocos" << std::endl;
    std::cout << " Piso de Segurança  : " << getSafetyFloor() << " (Checkpoint)" << std::endl;
    std::cout << " Dificuldade Atual  : " << difficulty << std::endl;
    std::cout << " Circulante (Supply): " << std::fixed << std::setprecision(8) << totalSupply << " MZ" << std::endl;
    std::cout << " Fundo de Reserva   : " << getBalance(MAZE_RESERVE_FUND) << " MZ" << std::endl;
    std::cout << " Supply Máximo      : " << getMaxSupply() << " MZ" << std::endl;
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
    if (from == MAZE_RESERVE_FUND) {
        throw std::runtime_error("ERRO: O Fundo de Reserva é gerido pelo sistema e não permite saques manuais.");
    }

    double currentFeePercent = getDynamicFeePercentage(static_cast<int>(chain.size()));
    double fee = amount * currentFeePercent; 
    double totalNeeded = amount + fee;

    // CHAMADA: MempoolAudit para verificar se o gasto duplo não está ocorrendo em transações não confirmadas
    if (!MempoolAudit::IsTransactionAllowed(*this, from, totalNeeded, MEMPOOL_PATH)) {
        throw std::runtime_error("Saldo insuficiente considerando transações pendentes no Mempool.");
    }

    // CHAMADA: MempoolLimit para gerir o tamanho da fila de transações
    std::vector<Transaction> pending = Storage::loadMempool(MEMPOOL_PATH);
    if (!MempoolLimit::CanAcceptTransaction(static_cast<int>(pending.size()))) {
        MempoolLimit::TrimMempool(pending); 
        if (!MempoolLimit::CanAcceptTransaction(static_cast<int>(pending.size()))) {
            throw std::runtime_error("Mempool atingiu o limite crítico. Aguarde o próximo bloco.");
        }
    }

    Crypto::KeyPair keys = Crypto::generate_keys_from_seed(seed);
    std::string data_to_sign = to + std::to_string(amount);

    Transaction tx;
    tx.id = Crypto::sha256d(from + to + std::to_string(amount) + std::to_string(std::time(nullptr)));

    tx.vout.push_back({to, amount}); 
    tx.vout.push_back({from, totalNeeded * -1}); 

    tx.publicKey = keys.public_key;
    tx.signature = Crypto::sign_data(data_to_sign, keys.private_key);

    Storage::saveMempool(tx, MEMPOOL_PATH);

    std::cout << "✅ Transação aceita! Enviando: " << amount << " MZ | Taxa: " << fee << " MZ" << std::endl;
}

void Blockchain::clearChain() { 
    chain.clear(); 
    totalSupply = 0; 
    utxoSet.utxoMap.clear(); 
    utxoSet.addressBalances.clear();
}

std::vector<Block> Blockchain::getChain() const { return chain; }
int Blockchain::getDifficulty() const { return difficulty; }
double Blockchain::getTotalSupply() const { return totalSupply; }
double Blockchain::getMaxSupply() const { return 20000000.0; }