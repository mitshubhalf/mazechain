#include "../include/block.h"
#include "../include/crypto.h"
#include "../include/blockchain.h" // Necessário para acessar o novo motor de hash
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

    // O hash inicial já nasce sob a nova regra do SHA-256d otimizado
    hash = calculateHash();
}

// --- IMPLEMENTAÇÃO DA MELHORIA: HEADER SERIALIZATION ---
// Esta função é vital para que o motor SHA256 receba os dados na ordem correta
std::string Block::toHashString() const {
    std::stringstream ss;

    // 1. Cálculo do Merkle Root (Mantendo sua lógica de transações)
    std::string root = Crypto::calculateMerkleRoot(this->transactions);

    // 2. Montagem do Cabeçalho (Block Header) conforme padrão de mercado
    // A inclusão de minerAddress e extraNonce no cabeçalho impede que dois 
    // mineradores trabalhem no mesmo hash exato.
    ss << index 
       << timestamp 
       << prevHash 
       << root
       << minerAddress
       << extraNonce
       << nonce;

    return ss.str();
}

std::string Block::calculateHash() const {
    // MUDANÇA CRUCIAL: Chamada ao motor CSHA256 (Double SHA256) 
    // definido na sua classe Blockchain.
    return Blockchain::calculateProofOfWork(toHashString());
}

void Block::mine(int difficulty) {
    // 1. Preparação do Alvo (Target)
    std::string target(difficulty, '0');

    std::cout << "[MINER] Iniciando Proof of Work (Motor: CSHA256 Bitcoin Core)" << std::endl;
    std::cout << "[MINER] Dificuldade: " << difficulty << " | Alvo: " << target << std::endl;

    // 2. Loop de Proof of Work (PoW)
    // Otimizado para alta performance com o novo motor de hash
    while (hash.substr(0, difficulty) != target) {
        nonce++;

        // Recalcula o hash do cabeçalho a cada iteração do nonce
        hash = calculateHash();

        // Monitoramento de progresso (Log a cada 100k tentativas)
        if (nonce % 100000 == 0) {
            std::cout << "[Nó] Minerando Bloco #" << index 
                      << " | Nonce: " << std::setw(10) << nonce 
                      << " | Hash parcial: " << hash.substr(0, 15) << "..." << std::endl;
        }
    }

    // 3. Resultado Final (Mantendo todas as informações de auditoria)
    std::cout << "🎯 Bloco #" << index << " minerado com sucesso!" << std::endl;
    std::cout << "   Hash Final:  " << hash << std::endl;
    std::cout << "   Nonce:       " << nonce << std::endl;
    std::cout << "   Minerador:   " << minerAddress << std::endl;
    std::cout << "   ExtraNonce:  " << extraNonce << std::endl;

    // O Merkle Root é exibido aqui para confirmar a integridade das transações incluídas
    std::string finalMerkle = Crypto::calculateMerkleRoot(this->transactions);
    std::cout << "   Root Merkle: " << finalMerkle << std::endl;
}