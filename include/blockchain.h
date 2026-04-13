#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <string>
#include "block.h"
#include "utxo.h"      // ADICIONADO: Para gerenciar saldos rápidos
#include "transaction.h"

class Blockchain {
private:
    std::vector<Block> chain;
    int difficulty;
    double totalSupply;

    // Métodos privados de auxílio (Regras de Rede)
    void adjustDifficulty();
    int getCurrentCycle(int height);

public:
    // --- ADICIONADO PARA O UTXO ---
    UTXOSet utxoSet; // Objeto que guarda quem tem moedas sem ler a chain toda

    Blockchain();
    
    // Gestão da Corrente e Mineração
    void mineBlock(std::string minerAddress);
    void addBlock(const Block& block);
    Block getLastBlock();
    void clearChain();
    
    // Lógica Econômica (Regras do MazeChain)
    double getBlockReward(int height);
    double getBalance(std::string address);
    double getTotalSupply() const { return totalSupply; }
    
    // Ações e Validação
    void send(std::string from, std::string to, double amount, std::string seed);
    bool verifyTransaction(const Transaction& tx);
    bool isChainValid();
    
    // Getters para a API (main.cpp)
    std::vector<Block> getChain() const;
    int getDifficulty() const;
    std::vector<Transaction> getMempool() const; 

    // Estatísticas
    void printStats();
};

#endif
