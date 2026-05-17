#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <string>
#include <atomic>
#include <mutex>
#include "block.h"
#include "utxo.h"      
#include "transaction.h"
#include "mining_utils.h" 
#include "crypto.h"

// Mutex global declarado como extern para ser acessado pelo .cpp
extern std::mutex g_blockchain_mutex;

class Blockchain {
private:
    std::vector<Block> chain;
    std::vector<Transaction> pendingTransactions; 

    int difficulty; // 🔥 ainda usado LOCALMENTE no miner
    unsigned int nBits; // 🔥 NOVO: dificuldade real estilo Bitcoin

    double totalSupply;
    const double MAX_SUPPLY = 20000000.0;

    // Sistema de controle para threads de mineração
    void rebuildUTXO(); 

    // Coinbase padronizada
    Transaction createCoinbaseTransaction(std::string minerAddress, double reward);

public:
    UTXOSet utxoSet; 

    // Controle de interrupção de mineração
    std::atomic<bool> interruptMining;

    Blockchain();

    // --- ECONOMIA ---
    double getDynamicFeePercentage(int height); 
    int getSafetyFloor(); 

    // --- MINERAÇÃO ---
    bool mineBlock(std::string minerAddress); 

    // 🔥 HASH (Bitcoin style double SHA256)
    static std::string calculateProofOfWork(const std::string& blockData);

    // 🔥 AGORA EXISTEM DUAS CAMADAS:
    void adjustDifficulty();        // (visual / compatibilidade)
    void adjustBits();              // 🔥 REAL CONSENSO (nBits)

    // --- BLOCK CONTROL ---
    void addBlock(const Block& block);
    Block getLastBlock() const { return chain.back(); }
    void clearChain();

    // --- ECONOMIA ---
    double getBlockReward(int height) const; // Adicionado const
    double getBalance(std::string address) const; // 🔥 CORRIGIDO: Adicionado const para evitar erro no Wallet
    double getTotalSupply() const;
    double getMaxSupply() const;

    // --- TRANSAÇÕES ---
    // Envia usando seed (método CLI legado)
    void send(std::string from, std::string to, double amount, std::string seed);

    // 🔥 NOVO: Método para buscar histórico de transações de um endereço
    void printAddressHistory(const std::string& targetAddress);

    // 🔥 NOVO: Adiciona transação assinada à mempool (Essencial para a API)
    // Mantido sem reduções, garantindo thread-safety
    bool addTransaction(const Transaction& tx) {
        std::lock_guard<std::mutex> lock(g_blockchain_mutex);
        if (verifyTransaction(tx)) {
            pendingTransactions.push_back(tx);
            return true;
        }
        return false;
    }

    bool verifyTransaction(const Transaction& tx);
    bool isChainValid();

    // --- ESTADO ---
    std::vector<Block> getChain() const;
    int getDifficulty() const; // ainda útil pra debug

    // 🔥 NOVO getter REAL
    unsigned int getBits() const { return nBits; }

    std::vector<Transaction> getPendingTransactions() const { return pendingTransactions; }

    // Altura atual
    int getHeight() const { return (int)chain.size(); }

    void printStats();
};

#endif