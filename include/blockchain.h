#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <string>
#include <atomic>
#include "block.h"
#include "utxo.h"      
#include "transaction.h"
#include "mining_utils.h" 
#include "crypto/sha256.h" // Importando o motor do Bitcoin

class Blockchain {
private:
    std::vector<Block> chain;
    std::vector<Transaction> pendingTransactions; 

    int difficulty;
    double totalSupply;
    const double MAX_SUPPLY = 20000000.0;

    // Sistema de controle para threads de mineração
    // Permite que múltiplas wallets minerem sem travar o nó
    void rebuildUTXO(); 

    // Função interna para criar a transação de recompensa (essencial para múltiplas wallets)
    Transaction createCoinbaseTransaction(std::string minerAddress, double reward);

public:
    UTXOSet utxoSet; 

    Blockchain();

    // --- FUNÇÕES DE SEGURANÇA E ECONOMIA ---
    double getDynamicFeePercentage(int height); 
    int getSafetyFloor(); 

    // --- LOGICA DE MINERAÇÃO PARA MÚLTIPLAS WALLETS ---
    // Alterado para retornar bool: permite saber se o bloco foi aceito
    bool mineBlock(std::string minerAddress); 

    // Função estática para calcular hash usando o CSHA256 do Bitcoin
    // É 'static' para que mineradores externos possam usar a mesma lógica
    static std::string calculateProofOfWork(const std::string& blockData);

    void adjustDifficulty(); 

    void addBlock(const Block& block);
    Block getLastBlock() const { return chain.back(); }
    void clearChain();

    // Getters de Economia
    double getBlockReward(int height);
    double getBalance(std::string address);
    double getTotalSupply() const;
    double getMaxSupply() const;

    // Sistema de Transações
    void send(std::string from, std::string to, double amount, std::string seed);
    bool verifyTransaction(const Transaction& tx);
    bool isChainValid();

    // Getters de Estado
    std::vector<Block> getChain() const;
    int getDifficulty() const;
    std::vector<Transaction> getPendingTransactions() const { return pendingTransactions; }

    // Retorna a altura atual da rede
    int getHeight() const { return (int)chain.size(); }

    void printStats();
};

#endif