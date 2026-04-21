#ifndef BLOCK_H
#define BLOCK_H

#include <vector>
#include <string>
#include <ctime>
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

    // NOVOS CAMPOS DE SEGURANÇA E UNICIDADE:
    std::string minerAddress;    // Endereço MZ de quem minerou o bloco
    long extraNonce;             // Valor aleatório para garantir trabalho único

    // Construtor atualizado: Agora recebe minerAddress e extraNonce para garantir a unicidade desde a criação
    Block(int idx, std::string prev, std::vector<Transaction> txs, std::string minerAddr, long eNonce);

    // Calcula o SHA256 combinando: index + timestamp + prevHash + nonce + merkleRoot + minerAddress + extraNonce
    std::string calculateHash() const;

    // O coração do minerador: busca um hash que comece com N zeros (dificuldade)
    // Agora o loop interno de mineração protege contra colisões usando os novos campos
    void mine(int difficulty); 
};

#endif