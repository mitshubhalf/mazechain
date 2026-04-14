#ifndef BLOCK_H
#define BLOCK_H

#include <vector>
#include <string>
#include <ctime>
#include "transaction.h"

class Block {
public:
    int index;
    long long timestamp;
    std::string hash;
    std::string prevHash;
    int nonce;
    std::vector<Transaction> transactions;

    Block(int idx, std::string prev, std::vector<Transaction> txs);
    
    // Calcula o hash combinando todos os dados do bloco
    std::string calculateHash() const;
    
    // Executa o Proof of Work internamente
    void mine(int difficulty); 
};

#endif
