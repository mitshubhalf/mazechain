#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <string>
#include "block.h"
#include "utxo.h"      
#include "transaction.h"

class Blockchain {
private:
    std::vector<Block> chain;
    int difficulty;
    double totalSupply;
    
    // Constante de limite máximo para garantir que a economia nunca ultrapasse 20M
    const double MAX_SUPPLY = 20000000.0;

    // Métodos privados de auxílio
    void adjustDifficulty();
    int getCurrentCycle(int height);

public:
    // UTXO SET: Saldo em cache (Vital para performance)
    UTXOSet utxoSet; 

    Blockchain();
    
    // --- GESTÃO DA CORRENTE E MINERAÇÃO ---
    void mineBlock(std::string minerAddress);
    void addBlock(const Block& block);
    Block getLastBlock() const { return chain.back(); }
    void clearChain();
    
    // --- LÓGICA ECONÔMICA (MAZECHAIN V2.1) ---
    // Regras: 1000 MZ (0-1k), 500 MZ (1k-2k), 250 MZ (2k-4k), Halving...
    double getBlockReward(int height);
    double getBalance(std::string address);
    double getTotalSupply() const { return totalSupply; }
    double getMaxSupply() const { return MAX_SUPPLY; } // Getter para conferência
    
    // --- AÇÕES E VALIDAÇÃO ---
    // Processa o envio, aplica taxa de 1% e salva em data/mempool.dat
    void send(std::string from, std::string to, double amount, std::string seed);
    
    // Valida se o remetente tem saldo, se a assinatura confere e o tamanho do endereço
    bool verifyTransaction(const Transaction& tx);
    
    // Varredura completa para garantir que ninguém alterou o histórico (PoW check)
    bool isChainValid();
    
    // --- INTEGRAÇÃO ---
    std::vector<Block> getChain() const;
    int getDifficulty() const;
    std::vector<Transaction> getMempool() const; 

    // Exibe Blocos, Supply Atual, Dificuldade e Taxas acumuladas
    void printStats();
};

#endif
