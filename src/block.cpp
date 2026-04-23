#include "../include/block.h"
#include "../include/crypto.h"
#include <iostream>
#include <sstream>
#include <ctime>
#include <iomanip>

// Construtor: Inicializa o bloco com os dados de identificação única do minerador
Block::Block(int idx, std::string prev, std::vector<Transaction> txs, std::string minerAddr, long eNonce) {
    index = idx;
    prevHash = prev;
    transactions = txs;
    minerAddress = minerAddr; 
    extraNonce = eNonce;      
    timestamp = std::time(0);
    nonce = 0;

    // O hash inicial já nasce sob a nova regra do SHA-256d
    hash = calculateHash();
}

std::string Block::calculateHash() const {
    std::stringstream ss;

    // 1. Cálculo do Merkle Root utilizando SHA-256d
    // Isso garante que se uma única transação for alterada, o Merkle Root muda completamente.
    std::string root = Crypto::calculateMerkleRoot(this->transactions);

    // 2. Montagem do Cabeçalho do Bloco (Block Header)
    // A ordem dos campos é vital para a imutabilidade. Incluímos o minerAddress e o 
    // extraNonce para garantir que dois mineradores nunca busquem o mesmo Hash (prevenindo colisão).
    ss << index 
       << timestamp 
       << prevHash 
       << root
       << minerAddress
       << extraNonce
       << nonce; // O nonce fica por último como o "contador" de tentativas

    // MUDANÇA CRUCIAL: Agora utilizamos sha256d (Double SHA) conforme o padrão Bitcoin
    return Crypto::sha256d(ss.str());
}

void Block::mine(int difficulty) {
    // 1. Preparação do Alvo (Target)
    // Geramos a string de zeros uma única vez fora do loop para máxima performance
    std::string target(difficulty, '0');

    std::cout << "[MINER] Iniciando Proof of Work (Algoritmo: SHA-256d)" << std::endl;
    std::cout << "[MINER] Dificuldade: " << difficulty << " | Alvo: " << target << std::endl;

    // 2. Loop de Proof of Work (PoW)
    // Este é o processo intensivo que valida o bloco. Usando SHA-256d, 
    // cada tentativa abaixo executa dois rounds de SHA-256 internamente.
    while (hash.substr(0, difficulty) != target) {
        nonce++;

        // Recalcula o hash duplo com o nonce incrementado
        hash = calculateHash();

        // Monitoramento de progresso a cada 100 mil tentativas
        if (nonce % 100000 == 0) {
            std::cout << "[Nó] Minerando Bloco #" << index 
                      << " | Nonce: " << std::setw(10) << nonce 
                      << " | Hash: " << hash.substr(0, 15) << "..." << std::endl;
        }
    }

    // 3. Resultado Final
    std::cout << "🎯 Bloco #" << index << " minerado com sucesso!" << std::endl;
    std::cout << "   Hash Final: " << hash << std::endl;
    std::cout << "   Nonce:      " << nonce << std::endl;
    std::cout << "   Minerador:  " << minerAddress << std::endl;
    std::cout << "   Root Merkle: " << Crypto::calculateMerkleRoot(this->transactions) << std::endl;
}