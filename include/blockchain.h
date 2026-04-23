#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <string>
#include "block.h"
#include "utxo.h"      
#include "transaction.h"
#include "mining_utils.h" 

class Blockchain {
private:
    std::vector<Block> chain;
    std::vector<Transaction> pendingTransactions; 

    int difficulty;
    double totalSupply;
    const double MAX_SUPPLY = 20000000.0;

    // Mantendo sua função de rebuild interna
    void rebuildUTXO(); 

public:
    UTXOSet utxoSet; 

    Blockchain();

    // Funções de lógica central (Públicas para serem acessadas pela API/Main)
    void mineBlock(std::string minerAddress); 
    void adjustDifficulty(); // Mudado para public para facilitar o gerenciamento do Nó

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

    void printStats();
};

#endif