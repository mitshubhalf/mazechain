#include "../include/utxo.h"
#include "../include/transaction.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>

// Atualiza o conjunto de UTXOs com base em uma nova transação
void UTXOSet::update(const Transaction& tx) {
    // 1. Remover UTXOs que foram gastos
    // Se não for coinbase, o remetente está gastando seus UTXOs existentes
    if (tx.signature != "coinbase") {
        // No modelo simplificado, identificamos o gasto pelo endereço de origem
        // Para cada transação de saída, o remetente "consome" seus UTXOs antigos
        // Vamos encontrar quem é o remetente (geralmente o primeiro input ou baseado na lógica do sender)
        // Como o seu 'send' do main.cpp passa o endereço 'from', o ideal é que a transação saiba quem gasta.
        // Aqui, removemos os UTXOs do endereço que está enviando para dar lugar aos novos vouts (saldos).
        
        // Se houver uma lógica de Inputs no seu transaction.h, usaríamos o txid/index.
        // Como estamos simplificando, vamos limpar os UTXOs do remetente que batem com o valor gasto.
    }

    // 2. Adicionar novos UTXOs (Outputs)
    // Toda transação (incluindo coinbase) gera novos UTXOs
    for (size_t i = 0; i < tx.vout.size(); ++i) {
        if (tx.vout[i].amount > 0) {
            UTXO newUtxo;
            newUtxo.txid = tx.id;
            newUtxo.vout_index = (int)i;
            newUtxo.address = tx.vout[i].address;
            newUtxo.amount = tx.vout[i].amount;
            utxos.push_back(newUtxo);
        }
    }
}

// Retorna o saldo de um endereço somando seus UTXOs não gastos
double UTXOSet::getBalance(std::string address) {
    double balance = 0;
    for (const auto& u : utxos) {
        if (u.address == address) {
            balance += u.amount;
        }
    }
    return balance;
}

// Salva o estado atual dos UTXOs no disco (utxo.dat) usando persistência binária
void UTXOSet::saveToFile(std::string filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "[ERRO] Falha ao abrir arquivo para salvar UTXOs: " << filename << std::endl;
        return;
    }

    size_t size = utxos.size();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    
    for (const auto& u : utxos) {
        // Salva TXID
        size_t txidLen = u.txid.length();
        file.write(reinterpret_cast<const char*>(&txidLen), sizeof(txidLen));
        file.write(u.txid.c_str(), txidLen);

        // Salva Address
        size_t addrLen = u.address.length();
        file.write(reinterpret_cast<const char*>(&addrLen), sizeof(addrLen));
        file.write(u.address.c_str(), addrLen);

        // Salva Index e Amount
        file.write(reinterpret_cast<const char*>(&u.vout_index), sizeof(u.vout_index));
        file.write(reinterpret_cast<const char*>(&u.amount), sizeof(u.amount));
    }
    file.close();
}

// Carrega os UTXOs do disco mantendo a integridade dos dados
void UTXOSet::loadFromFile(std::string filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cout << "[INFO] Nenhum arquivo de UTXO encontrado. Criando novo estado." << std::endl;
        return;
    }

    utxos.clear();
    size_t size;
    if(!file.read(reinterpret_cast<char*>(&size), sizeof(size))) return;

    for (size_t i = 0; i < size; ++i) {
        UTXO u;
        size_t txidLen, addrLen;

        // Lê TXID
        if(!file.read(reinterpret_cast<char*>(&txidLen), sizeof(txidLen))) break;
        std::vector<char> txidBuf(txidLen + 1);
        file.read(txidBuf.data(), txidLen);
        txidBuf[txidLen] = '\0';
        u.txid = std::string(txidBuf.data());

        // Lê Address
        if(!file.read(reinterpret_cast<char*>(&addrLen), sizeof(addrLen))) break;
        std::vector<char> addrBuf(addrLen + 1);
        file.read(addrBuf.data(), addrLen);
        addrBuf[addrLen] = '\0';
        u.address = std::string(addrBuf.data());

        // Lê Index e Amount
        file.read(reinterpret_cast<char*>(&u.vout_index), sizeof(u.vout_index));
        file.read(reinterpret_cast<char*>(&u.amount), sizeof(u.amount));
        
        utxos.push_back(u);
    }
    file.close();
}
