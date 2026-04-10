// include/blockchain.h
#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <string>
#include <vector>

struct VOut {
    std::string address;
    double amount;
};

struct Transaction {
    std::vector<std::string> vin; // Entradas (simplificadas)
    std::vector<VOut> vout;       // Saídas
};

class Block {
public:
    int index;
    long timestamp;
    std::string hash;
    std::string prevHash; // Certifique-se que aqui é prevHash
    long nonce;
    std::vector<Transaction> transactions;

    Block(int idx, std::string prev, std::vector<Transaction> txs);
    std::string calculateHash() const;
    void mine(int difficulty);
};

// ... restante da classe Blockchain
#endif
