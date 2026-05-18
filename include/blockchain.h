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

// ── Constantes de Consenso (Protocol v4.0) ────────────────────────────────────
static const int    MAX_BLOCK_SIZE     = 1000000;  // 1 MB por bloco
static const int    MAX_BLOCK_TXS      = 5000;     // máx. transações por bloco
static const int    MAX_CLOCK_DRIFT    = 7200;     // blocos futuros: máx. 2h adiantados
static const int    MEDIAN_TIME_WINDOW = 11;       // janela para Median Time Past
static const double MAX_SUPPLY         = 20000000.0;

class Blockchain {
private:
    std::vector<Block> chain;
    std::vector<Transaction> pendingTransactions; 

    int difficulty;
    unsigned int nBits;

    double totalSupply;

    // Sistema de controle para threads de mineração
    void rebuildUTXO(); 

    // Coinbase padronizada
    Transaction createCoinbaseTransaction(std::string minerAddress, double reward);

    // Calcula a Median Time Past (mediana dos últimos MEDIAN_TIME_WINDOW timestamps)
    long long getMedianTimePast() const;

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

    // Hash estilo Bitcoin (double SHA256)
    static std::string calculateProofOfWork(const std::string& blockData);

    void adjustDifficulty();
    void adjustBits();

    // --- BLOCK CONTROL ---
    // trusted=true: carregamento do disco (pula validação de timestamp/tamanho)
    // trusted=false: bloco recebido de peer (valida tudo)
    void addBlock(const Block& block, bool trusted = false);
    Block getLastBlock() const { return chain.back(); }
    void clearChain();

    // ── NOVO: Longest Chain Rule + Reorg ─────────────────────────────────────
    // Substitui a cadeia local se newChain for válida e maior.
    // Reconstrói o UTXO set automaticamente após o reorg.
    bool replaceChain(const std::vector<Block>& newChain);

    // Retorna o "chain work" = número de blocos (proxy simples, sem acúmulo de bits)
    long long getChainWork() const { return (long long)chain.size(); }

    // --- ECONOMIA ---
    double getBlockReward(int height) const;
    double getBalance(std::string address) const;
    double getTotalSupply() const;
    double getMaxSupply() const;

    // --- TRANSAÇÕES ---
    void send(std::string from, std::string to, double amount, std::string seed);
    void printAddressHistory(const std::string& targetAddress);

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
    int getDifficulty() const;
    unsigned int getBits() const { return nBits; }
    std::vector<Transaction> getPendingTransactions() const { return pendingTransactions; }
    int getHeight() const { return (int)chain.size(); }
    void printStats();
};

#endif