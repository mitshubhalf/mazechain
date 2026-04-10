#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <string>
#include <vector>

struct VOut {
    std::string address;
    double amount;
};

struct Transaction {
    std::vector<std::string> vin;
    std::vector<VOut> vout;

    // Adicione esses construtores para aceitar a sintaxe de chaves { }
    Transaction() {}
    Transaction(std::vector<std::string> i, std::vector<VOut> o) : vin(i), vout(o) {}
};

class Block {
public:
    int index;
    long timestamp;
    std::string hash;
    std::string prevHash; 
    long nonce;
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
    const int DIFFICULTY_ADJUSTMENT_INTERVAL = 10;
    const int TARGET_BLOCK_TIME = 30;

public:
    Blockchain();
    void mineBlock(std::string minerAddress);
    double getBalance(std::string address);
    void send(std::string from, std::string to, double amount);
    
    Block getLastBlock();
    double getBlockReward(int height);
    void adjustDifficulty();
    
    std::vector<Block> getChain() const;
    int getDifficulty() const;
    void setDifficulty(int d);
    void clearChain();
    void addBlock(const Block& block);
};

#endif
