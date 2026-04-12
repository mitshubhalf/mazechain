#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <string>
#include <vector>
#include <ctime>

struct TxOut {
    std::string address;
    double amount;
};

struct Transaction {
    std::string id;
    std::string signature;
    std::string publicKey;
    std::vector<TxOut> vout;
};

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

class Blockchain {
private:
    std::vector<Block> chain;
    int difficulty;
    double totalSupply;

public:
    Blockchain();
    void mineBlock(std::string minerAddress);
    void addBlock(const Block& block);
    std::vector<Block> getChain() const;
    int getDifficulty() const;
    void setDifficulty(int d);
    double getBalance(std::string address);
    void send(std::string from, std::string to, double amount, std::string seed);
    bool verifyTransaction(const Transaction& tx);
    void adjustDifficulty();
    double getBlockReward(int height);
    
    // --- FUNÇÃO ADICIONADA PARA SINCRONIZAR COM O CPP ---
    int getCurrentCycle(int height); 
    
    Block getLastBlock();
    void clearChain();
    bool isChainValid();
    bool isChainValid(const std::vector<Block>& chainToValidate);
    void replaceChain(const std::vector<Block>& newChain);
    void printStats();
};

// Funções utilitárias globais
std::string sha256_util(std::string str);
std::string calculateMerkleRoot(const std::vector<Transaction>& txs);

#endif
