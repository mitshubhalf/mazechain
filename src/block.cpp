#include "../include/block.h"
#include "../include/crypto.h"
#include <iostream>
#include <sstream>
#include <ctime>

Block::Block(int idx, std::string prev, std::vector<Transaction> txs) {
    index = idx;
    prevHash = prev;
    transactions = txs;
    timestamp = std::time(0);
    nonce = 0;
    hash = calculateHash();
}

std::string Block::calculateHash() const {
    std::stringstream ss;
    
    // O Merkle Root garante que se uma única transação for alterada, 
    // o hash do bloco inteiro muda instantaneamente.
    std::string root = Crypto::calculateMerkleRoot(this->transactions);
    
    ss << index 
       << static_cast<long long>(timestamp) 
       << prevHash 
       << nonce 
       << root;
       
    return Crypto::sha256_util(ss.str());
}

void Block::mine(int difficulty) {
    // Define o alvo (ex: "0000" para dificuldade 4)
    std::string target(difficulty, '0');
    
    // O loop de Proof of Work (Trabalho Computacional)
    while (hash.substr(0, difficulty) != target) {
        nonce++;
        hash = calculateHash();
        
        // Log opcional para monitorar o progresso em blocos pesados
        if (nonce % 100000 == 0) {
            std::cout << "[Nó] Minerando... Nonce: " << nonce << " | Hash parcial: " << hash.substr(0, 10) << "..." << std::endl;
        }
    }
    
    std::cout << "🎯 Bloco #" << index << " minerado com sucesso! " << std::endl;
    std::cout << "   Hash: " << hash << std::endl;
    std::cout << "   Nonce: " << nonce << std::endl;
}
