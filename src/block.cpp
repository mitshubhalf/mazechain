#include "../include/block.h"
#include "../include/crypto.h"
#include <iostream>
#include <sstream>
#include <ctime>
#include <iomanip>

Block::Block(int idx, std::string prev, std::vector<Transaction> txs) {
    index = idx;
    prevHash = prev;
    transactions = txs;
    timestamp = std::time(0);
    nonce = 0;
    // O hash inicial já deve considerar o Merkle Root das transações
    hash = calculateHash();
}

std::string Block::calculateHash() const {
    std::stringstream ss;
    
    // 1. Cálculo do Merkle Root: 
    // Essencial para a integridade. Se mudar o valor de uma taxa 
    // ou um endereço de destino, o Root muda e o Hash quebra.
    std::string root = Crypto::calculateMerkleRoot(this->transactions);
    
    // 2. Montagem do Cabeçalho do Bloco (Block Header)
    // Usamos o timestamp fixo capturado no construtor para o nonce não "competir" com o tempo
    ss << index 
       << timestamp 
       << prevHash 
       << nonce 
       << root;
       
    return Crypto::sha256_util(ss.str());
}

void Block::mine(int difficulty) {
    // Define o alvo (target) baseado na dificuldade atual da rede
    // Ex: Dificuldade 4 -> target = "0000"
    std::string target(difficulty, '0');
    
    // Registro do início para cálculo de performance (opcional)
    std::cout << "[MINER] Iniciando busca pelo Hash válido (Dificuldade: " << difficulty << ")" << std::endl;

    // --- LOOP DE PROOF OF WORK (PoW) ---
    // É aqui que o seu computador gasta energia para validar a rede.
    while (hash.substr(0, difficulty) != target) {
        nonce++;
        
        // Recalcula o hash com o novo nonce
        hash = calculateHash();
        
        // Monitoramento de progresso
        if (nonce % 100000 == 0) {
            std::cout << "[Nó] Minerando Bloco #" << index 
                      << " | Nonce: " << nonce 
                      << " | Hash: " << hash.substr(0, 10) << "..." << std::endl;
        }
    }
    
    std::cout << "🎯 Bloco #" << index << " minerado com sucesso!" << std::endl;
    std::cout << "   Hash: " << hash << std::endl;
    std::cout << "   Nonce: " << nonce << std::endl;
    std::cout << "   Transações: " << transactions.size() << " incluídas." << std::endl;
}
