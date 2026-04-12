#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <string>
#include <vector>
#include <ctime>

// Estrutura de saída de transação
struct TxOut {
    std::string address;
    double amount;
};

// Estrutura de transação
struct Transaction {
    std::string id;
    std::string signature;
    std::string publicKey;
    std::vector<TxOut> vout;
};

// Classe do Bloco
class Block {
public:
    int index;
    long long timestamp;
    std::string hash;
    std::string prevHash;
    int nonce;
    std::vector<Transaction> transactions;

    Block(int idx, std::string prev, std::vector<Transaction> txs);
    std::string calculateHash() const;
    void mine(int difficulty);
};

// Classe da Blockchain
class Blockchain {
private:
    std::vector<Block> chain;
    int difficulty;
    double totalSupply;

public:
    Blockchain();
    
    // Funções de Mineração e Blocos
    void mineBlock(std::string minerAddress);
    void addBlock(const Block& block);
    Block getLastBlock();
    void clearChain();
    
    // Getters
    std::vector<Block> getChain() const;
    int getDifficulty() const;
    void setDifficulty(int d);
    double getTotalSupply() const { return totalSupply; }
    
    // --- CORREÇÃO: Função necessária para o main.cpp ---
    // Esta função busca as transações pendentes para a API
    std::vector<Transaction> getMempool() const; 

    // Lógica Financeira e Regras
    double getBalance(std::string address);
    void send(std::string from, std::string to, double amount, std::string seed);
    bool verifyTransaction(const Transaction& tx);
    double getBlockReward(int height);
    
    // Ajustes de Rede
    void adjustDifficulty();
    int getCurrentCycle(int height); 
    
    // Validação
    bool isChainValid();
    bool isChainValid(const std::vector<Block>& chainToValidate);
    void replaceChain(const std::vector<Block>& newChain);
    
    // Estatísticas
    void printStats();
};

// Funções utilitárias globais
std::string sha256_util(std::string str);
std::string calculateMerkleRoot(const std::vector<Transaction>& txs);

#endif
