#ifndef BLOCK_H
#define BLOCK_H

#include <vector>
#include <string>
#include <ctime>
#include <atomic> // Necessário para a flag de interrupção
#include "transaction.h"

class Block {
public:
    int index;                   // Altura do bloco na rede
    long long timestamp;         // Momento da criação
    std::string hash;            // Hash do cabeçalho do bloco (ID do bloco)
    std::string prevHash;        // Link para o bloco anterior
    std::string merkleRoot;      // Resumo criptográfico de todas as transações
    int nonce;                   // O número "mágico" do Proof of Work
    std::vector<Transaction> transactions; // Lista de transações (incluindo a Coinbase)

    // CAMPOS DE SEGURANÇA E UNICIDADE:
    std::string minerAddress;    // Endereço MZ de quem minerou o bloco
    long extraNonce;             // Valor aleatório para garantir trabalho único

    // Construtor atualizado: Recebe minerAddress e extraNonce para garantir a unicidade
    Block(int idx, std::string prev, std::vector<Transaction> txs, std::string minerAddr, long eNonce);

    // Concatena os dados do bloco em uma string para ser processada pelo SHA256
    // Essencial para o funcionamento do motor CSHA256 externo
    std::string toHashString() const;

    // Calcula o SHA256 combinando os dados do cabeçalho
    std::string calculateHash() const;

    // O coração do minerador clássico
    void mine(int difficulty); 

    // --- MELHORIA CRÍTICA PARA CONCURRÊNCIA ---
    // Esta função permite que o motor de mineração pare se a rede mudar (evita o erro do bloco #5000)
    bool mineWithInterrupt(int difficulty, std::atomic<bool>& interrupt);
};

#endif