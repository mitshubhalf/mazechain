#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <string>
#include "block.h"
#include "utxo.h"      
#include "transaction.h"
#include "mining_utils.h" // Inclusão necessária para MinerIdentity

class Blockchain {
private:
    std::vector<Block> chain;
    std::vector<Transaction> pendingTransactions; 

    int difficulty;
    double totalSupply;
    const double MAX_SUPPLY = 20000000.0;

    void adjustDifficulty();
    int getCurrentCycle(int height);

    // Mantendo sua função de rebuild
    void rebuildUTXO(); 

    // NOVO: Função interna de suporte para calcular o hash com os novos campos de segurança
    std::string calculateBlockHash(int index, std::string prevHash, long timestamp, 
                                 const std::vector<Transaction>& txs, int nonce, 
                                 std::string minerAddr, long extraNonce);

public:
    UTXOSet utxoSet; 

    Blockchain();

    // A assinatura continua a mesma para o seu Crow (main.cpp) não quebrar
    void mineBlock(std::string minerAddress); 

    void addBlock(const Block& block);
    Block getLastBlock() const { return chain.back(); }
    void clearChain();

    double getBlockReward(int height);
    double getBalance(std::string address);

    double getTotalSupply() const;
    double getMaxSupply() const;

    void send(std::string from, std::string to, double amount, std::string seed);
    bool verifyTransaction(const Transaction& tx);
    bool isChainValid();

    std::vector<Block> getChain() const;
    int getDifficulty() const;
    std::vector<Transaction> getPendingTransactions() const { return pendingTransactions; }

    void printStats();
};

#endif