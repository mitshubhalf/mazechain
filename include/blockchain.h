#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>

struct TxOut {
    std::string address;
    double amount;
};

struct TxIn {
    std::string prevTxId;
    int outIndex;
};

struct Transaction {
    std::string id;
    std::vector<TxIn> vin;
    std::vector<TxOut> vout;
    std::string signature;
    std::string publicKey;

    Transaction() {}
    Transaction(std::vector<TxIn> in, std::vector<TxOut> out) : vin(in), vout(out) {}
};

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

class Blockchain {
private:
    std::vector<Block> chain;
    int difficulty;
    double totalSupply;
    const int DIFFICULTY_ADJUSTMENT_INTERVAL = 10;
    const int TARGET_BLOCK_TIME = 60;

public:
    Blockchain();
    void mineBlock(std::string minerAddress);
    double getBalance(std::string address);
    void send(std::string from, std::string to, double amount, std::string seed = "");
    bool isChainValid();
    bool isChainValid(const std::vector<Block>& chainToValidate);
    bool verifyTransaction(const Transaction& tx); 
    Block getLastBlock();
    double getBlockReward(int height);
    void adjustDifficulty();
    void printStats();
    
    // FUNÇÕES DE UTILIDADE E REDE
    void clearChain(); // <--- ESSENCIAL: Resolve o erro do log
    std::vector<Block> getChain() const;
    int getDifficulty() const;
    void setDifficulty(int d);
    void replaceChain(const std::vector<Block>& newChain);
    void addBlock(const Block& block);
};

std::string sha256_util(std::string str);
std::string calculateMerkleRoot(const std::vector<Transaction>& txs);

#endif
