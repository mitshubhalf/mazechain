#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>

// Estrutura para saídas de transação (quem recebe e quanto)
struct TxOut {
    std::string address;
    double amount;
};

// Estrutura para entradas de transação (referência a moedas passadas)
struct TxIn {
    std::string prevTxId;
    int outIndex;
};

// Estrutura da Transação (Atualizada para Segurança Digital)
struct Transaction {
    std::string id;
    std::vector<TxIn> vin;
    std::vector<TxOut> vout;
    
    // CAMPOS CRÍTICOS PARA ASSINATURA ECDSA
    std::string signature;  // O "carimbo" digital gerado pela chave privada
    std::string publicKey;  // A chave pública para o minerador validar o carimbo

    Transaction() {}
    Transaction(std::vector<TxIn> in, std::vector<TxOut> out) : vin(in), vout(out) {}
};

// Classe do Bloco
class Block {
public:
    int index;
    long long timestamp;
    std::string prevHash;
    std::string hash;
    int nonce;
    std::vector<Transaction> transactions;

    Block(int idx, std::string prev, std::vector<Transaction> txs);
    std::string calculateHash() const;
    void mine(int difficulty);
};

// Classe da Blockchain (Gerenciamento da Rede)
class Blockchain {
private:
    std::vector<Block> chain;
    int difficulty;
    double totalSupply;
    const int DIFFICULTY_ADJUSTMENT_INTERVAL = 10;
    const int TARGET_BLOCK_TIME = 60;

public:
    Blockchain();
    
    // Funções de Mineração e Core
    void mineBlock(std::string minerAddress);
    double getBalance(std::string address);
    void send(std::string from, std::string to, double amount);
    
    // Funções de Validação e Segurança
    bool isChainValid();
    bool verifyTransaction(const Transaction& tx); // Nova função para validar assinaturas
    
    // Getters e Utilidades
    Block getLastBlock();
    double getBlockReward(int height);
    void adjustDifficulty();
    void printStats();
    void printBlockDetails(int height);
    
    std::vector<Block> getChain() const;
    int getDifficulty() const;
    void setDifficulty(int d);
    void clearChain();
    void addBlock(const Block& block);
};

// Utilitário global para Hash
std::string sha256_util(std::string str);

#endif
