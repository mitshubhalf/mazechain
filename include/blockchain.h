#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <string>
#include "block.h"
#include "utxo.h"      // Gerencia saldos rápidos via UTXO Set
#include "transaction.h"

class Blockchain {
private:
    std::vector<Block> chain;
    int difficulty;
    double totalSupply;

    // Métodos privados de auxílio (Regras de Protocolo MazeChain)
    void adjustDifficulty();
    int getCurrentCycle(int height);

public:
    // --- UTXO SET ---
    // Objeto que armazena os saldos atuais em cache para evitar varredura total da chain
    UTXOSet utxoSet; 

    Blockchain();
    
    // --- GESTÃO DA CORRENTE E MINERAÇÃO ---
    // Responsável por processar a mempool, validar taxas e criar novos blocos
    void mineBlock(std::string minerAddress);
    
    // Adiciona o bloco à corrente e dispara a atualização do UTXO Set
    void addBlock(const Block& block);
    
    // Retorna o último bloco minerado na rede
    Block getLastBlock() const { return chain.back(); }
    
    // Reseta a blockchain (limpa memória e arquivos temporários)
    void clearChain();
    
    // --- LÓGICA ECONÔMICA (REGRAS DO MAZECHAIN) ---
    // Calcula o subsídio do bloco baseado na altura e halving
    double getBlockReward(int height);
    
    // Consulta o saldo atual via UTXO Set (Otimizado)
    double getBalance(std::string address);
    
    // Retorna a quantidade total de moedas em circulação
    double getTotalSupply() const { return totalSupply; }
    
    // --- AÇÕES E VALIDAÇÃO ---
    // Cria uma transação, calcula taxas (1%) e envia para o arquivo mempool.dat
    void send(std::string from, std::string to, double amount, std::string seed);
    
    // Valida assinaturas e regras de maturidade (Coinbase Maturity)
    bool verifyTransaction(const Transaction& tx);
    
    // Verifica a integridade matemática da blockchain (hashes e hashes anteriores)
    bool isChainValid();
    
    // --- GETTERS PARA A API (MAIN.CPP) ---
    std::vector<Block> getChain() const;
    int getDifficulty() const;
    
    // Carrega as transações pendentes diretamente do armazenamento
    std::vector<Transaction> getMempool() const; 

    // --- ESTATÍSTICAS NO CONSOLE ---
    void printStats();
};

#endif
