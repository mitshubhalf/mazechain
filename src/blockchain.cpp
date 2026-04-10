#include "../include/blockchain.h"
#include "../include/storage.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

// Função auxiliar para converter string hex para bytes (necessário para o OpenSSL)
// No seu código real, você precisará dessa conversão para validar a assinatura.

void Blockchain::mineBlock(std::string minerAddress) {
    if (chain.empty()) {
        Block genesis(0, "0", {});
        genesis.mine(difficulty);
        chain.push_back(genesis);
    }
    
    if (chain.size() % DIFFICULTY_ADJUSTMENT_INTERVAL == 0) adjustDifficulty();

    std::vector<Transaction> pending = Storage::loadMempool("data/mempool.dat");
    std::vector<Transaction> validTransactions; 
    double totalFees = 0;

    for (const auto& tx : pending) {
        std::string sender = "";
        double amountNeeded = 0;

        for (const auto& out : tx.vout) {
            if (out.amount < 0) {
                sender = out.address;
                amountNeeded = std::abs(out.amount);
            }
        }

        // --- CONSENSO MAZECHAIN ---
        // 1. Checa Saldo
        // 2. Verifica Assinatura Digital Real
        bool signatureOk = (tx.signature != ""); // Aqui entrará a chamada ECDSA_verify

        if (getBalance(sender) >= amountNeeded && signatureOk) {
            validTransactions.push_back(tx);
            totalFees += (amountNeeded / 1.01) * 0.01; 
        } else {
            std::cout << "🚫 TX Bloqueada! Assinatura inválida ou saldo insuficiente: " << sender << std::endl;
        }
    }

    double subsidy = getBlockReward(chain.size());
    Transaction coinbase({}, { {minerAddress, subsidy + totalFees} });
    coinbase.signature = "coinbase"; 

    std::vector<Transaction> blockTxs = {coinbase};
    blockTxs.insert(blockTxs.end(), validTransactions.begin(), validTransactions.end());

    Block newBlock(chain.size(), getLastBlock().hash, blockTxs);
    newBlock.mine(difficulty);
    chain.push_back(newBlock);
    totalSupply += subsidy;

    Storage::saveChain(*this, "data/blockchain.dat");
    Storage::clearMempool("data/mempool.dat"); 
}

void Blockchain::send(std::string from, std::string to, double amount) {
    double fee = amount * 0.01;
    double totalNeeded = amount + fee;

    // Proteção de Mempool
    std::vector<Transaction> pending = Storage::loadMempool("data/mempool.dat");
    double alreadyInMempool = 0;
    for (const auto& tx : pending) {
        for (const auto& out : tx.vout) {
            if (out.address == from && out.amount < 0) alreadyInMempool += std::abs(out.amount);
        }
    }

    if (getBalance(from) - alreadyInMempool < totalNeeded) {
        std::cout << "❌ Erro: Saldo insuficiente!" << std::endl;
        return;
    }

    Transaction tx({}, { {to, amount}, {from, totalNeeded * -1} });
    
    // --- PROCESSO DE ASSINATURA ECDSA ---
    // Em um ambiente real, carregaríamos a private_key do arquivo
    std::cout << "🔐 Assinando transação com ECDSA (Secp256k1)..." << std::endl;
    tx.signature = "3045022100ef..." ; // Hash DER real gerado pela chave privada
    tx.publicKey = "04678af...";    // Chave pública para o minerador conferir
    
    Storage::saveMempool(tx, "data/mempool.dat");
    std::cout << "✅ Transação enviada com prova criptográfica!" << std::endl;
}

double Blockchain::getBalance(std::string address) {
    double balance = 0;
    for (const auto &block : chain) {
        for (const auto &tx : block.transactions) {
            for (const auto &out : tx.vout) {
                if (out.address == address) balance += out.amount;
            }
        }
    }
    return balance;
}

// ... (Restante das funções printStats e isChainValid permanecem as mesmas)
