#include "../include/utxo.h"
#include "../include/transaction.h"
#include <fstream>
#include <iostream>
#include <algorithm>

// Atualiza o conjunto de UTXOs com base em uma nova transação
void UTXOSet::update(const Transaction& tx) {
    // 1. Remover UTXOs que foram gastos (Inputs)
    // Se a transação não for coinbase, ela deve ter entradas que gastam UTXOs existentes
    if (tx.signature != "coinbase") {
        for (const auto& out : tx.vout) {
            if (out.amount < 0) { // No seu modelo, valores negativos representam gasto
                auto it = std::remove_if(utxos.begin(), utxos.end(), [&](const UTXO& u) {
                    return u.address == out.address && std::abs(u.amount - std::abs(out.amount)) < 0.000001;
                });
                utxos.erase(it, utxos.end());
            }
        }
    }

    // 2. Adicionar novos UTXOs (Outputs positivos)
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

// Retorna o saldo de um endereço de forma instantânea
double UTXOSet::getBalance(std::string address) {
    double balance = 0;
    for (const auto& u : utxos) {
        if (u.address == address) {
            balance += u.amount;
        }
    }
    return balance;
}

// Salva o estado atual dos UTXOs no disco (utxo.dat)
void UTXOSet::saveToFile(std::string filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) return;

    size_t size = utxos.size();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    
    for (const auto& u : utxos) {
        // Salva o tamanho da string antes da string em si
        size_t txidLen = u.txid.length();
        file.write(reinterpret_cast<const char*>(&txidLen), sizeof(txidLen));
        file.write(u.txid.c_str(), txidLen);

        size_t addrLen = u.address.length();
        file.write(reinterpret_cast<const char*>(&addrLen), sizeof(addrLen));
        file.write(u.address.c_str(), addrLen);

        file.write(reinterpret_cast<const char*>(&u.vout_index), sizeof(u.vout_index));
        file.write(reinterpret_cast<const char*>(&u.amount), sizeof(u.amount));
    }
    file.close();
}

// Carrega os UTXOs do disco
void UTXOSet::loadFromFile(std::string filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return;

    utxos.clear();
    size_t size;
    file.read(reinterpret_cast<char*>(&size), sizeof(size));

    for (size_t i = 0; i < size; ++i) {
        UTXO u;
        size_t txidLen, addrLen;

        file.read(reinterpret_cast<char*>(&txidLen), sizeof(txidLen));
        char* txidBuf = new char[txidLen + 1];
        file.read(txidBuf, txidLen);
        txidBuf[txidLen] = '\0';
        u.txid = txidBuf;
        delete[] txidBuf;

        file.read(reinterpret_cast<char*>(&addrLen), sizeof(addrLen));
        char* addrBuf = new char[addrLen + 1];
        file.read(addrBuf, addrLen);
        addrBuf[addrLen] = '\0';
        u.address = addrBuf;
        delete[] addrBuf;

        file.read(reinterpret_cast<char*>(&u.vout_index), sizeof(u.vout_index));
        file.read(reinterpret_cast<char*>(&u.amount), sizeof(u.amount));
        utxos.push_back(u);
    }
    file.close();
}
