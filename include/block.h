#ifndef BLOCK_H
#define BLOCK_H

#include <vector>
#include <string>
#include <ctime>
#include "transaction.h"

class Block {
public:
    int index;                  // Altura do bloco na rede
    long long timestamp;        // Momento da criação
    std::string hash;           // Hash do cabeçalho do bloco (ID do bloco)
    std::string prevHash;       // Link para o bloco anterior
    std::string merkleRoot;     // Resumo criptográfico de todas as transações
    int nonce;                  // O número "mágico" do Proof of Work
    std::vector<Transaction> transactions; // Lista de transações (incluindo a Coinbase)

    // Construtor: Inicializa o bloco com os dados fornecidos
    Block(int idx, std::string prev, std::vector<Transaction> txs);
    
    // Calcula o SHA256 combinando: index + timestamp + prevHash + nonce + merkleRoot
    std::string calculateHash() const;
    
    // O coração do minerador: busca um hash que comece com N zeros (dificuldade)
    void mine(int difficulty); 
};

#endif
