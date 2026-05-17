#include "../include/block.h"
#include "../include/crypto.h"
#include "../include/blockchain.h"
#include <iostream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <atomic>

// Construtor completo: Inicializa o bloco com os dados de consenso
Block::Block(int idx, std::string prev, std::vector<Transaction> txs, std::string minerAddr, long eNonce) {
    index = idx;
    prevHash = prev;
    transactions = txs;
    minerAddress = minerAddr; 
    extraNonce = eNonce;      
    timestamp = std::time(0);
    nonce = 0;

    // Inicializa o hash com base nos dados iniciais
    hash = calculateHash();
}

// Serialização do cabeçalho para o Proof of Work (PoW)
std::string Block::toHashString() const {
    std::stringstream ss;

    // O Merkle Root vincula todas as transações ao cabeçalho do bloco
    std::string root = Crypto::calculateMerkleRoot(this->transactions);

    // Ordem estrita dos campos para garantir determinismo no hash
    ss << index 
       << timestamp 
       << prevHash 
       << root
       << minerAddress
       << extraNonce
       << nonce;

    return ss.str();
}

// Cálculo de Hash Duplo (SHA-256d) para máxima segurança
std::string Block::calculateHash() const {
    // Utiliza a função double-sha256 do motor MazeChain
    return Crypto::sha256d(toHashString()); 
}

// Lógica de Mineração (Proof of Work) com Suporte a Escada de Dificuldade
bool Block::mineWithInterrupt(int difficulty, std::atomic<bool>& interrupt) {
    // Define o padrão de zeros exigido (Ex: Dificuldade 5 = "00000")
    std::string target(difficulty, '0');

    std::cout << "[MOTOR] ⛏️ Minerando Bloco #" << index 
              << " | Dificuldade Atual: " << difficulty << std::endl;

    // Loop de busca da solução (Hash que satisfaça o Target)
    while (hash.substr(0, difficulty) != target) {

        // 1. Verificação de Interrupção
        // Se a rede encontrar o bloco antes de nós, paramos imediatamente
        if (interrupt.load()) {
            std::cout << "[MOTOR] 🛑 Mineração do bloco #" << index << " abortada (Novo bloco detectado na rede)." << std::endl;
            return false;
        }

        // Incremento do Nonce primário
        nonce++;

        // 2. Renovação de Entropia (Caso o Nonce atinja o limite de 32 bits)
        if (nonce == 0) { 
            extraNonce++;
            timestamp = std::time(0); // Atualiza o tempo para mudar o hash base
            std::cout << "[MOTOR] 🔄 Ciclo de Nonce completo. ExtraNonce incrementado para: " << extraNonce << std::endl;
        }

        // Recalcula o hash com os novos valores de nonce/timestamp
        hash = calculateHash();

        // 3. Feedback de progresso no Terminal
        if (nonce % 1000000 == 0) {
            std::cout << "[Nó] Altura: " << index 
                      << " | Nonce: " << std::setw(10) << nonce 
                      << " | Hash: " << hash.substr(0, 12) << "..." << std::endl;
        }
    }

    // 4. Relatório de Sucesso da Mineração
    std::cout << "\n🎯 Bloco #" << index << " MINERADO COM SUCESSO!" << std::endl;
    std::cout << "   Hash Final:    " << hash << std::endl;
    std::cout << "   Dificuldade:   " << difficulty << std::endl;
    std::cout << "   Nonce Final:   " << nonce << std::endl;
    std::cout << "   ExtraNonce:    " << extraNonce << std::endl;

    // Validação de integridade pós-mineração
    std::string finalMerkle = Crypto::calculateMerkleRoot(this->transactions);
    std::cout << "   Merkle Root:   " << finalMerkle << std::endl;

    return true; 
}