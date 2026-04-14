#include "../include/utxo.h"
#include "../include/transaction.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>

// Atualiza o conjunto de UTXOs com base em uma nova transação
void UTXOSet::update(const Transaction& tx) {
    // Processamos cada saída (vout) da transação
    for (const auto& out : tx.vout) {
        if (out.amount > 0) {
            // Se o valor é positivo, é um NOVO saldo (Crédito)
            UTXO newUtxo;
            newUtxo.txid = tx.id;
            newUtxo.address = out.address;
            newUtxo.amount = out.amount;
            newUtxo.vout_index = 0; // Simplificado para este modelo
            utxos.push_back(newUtxo);
        } 
        else if (out.amount < 0) {
            // Se o valor é negativo, é um GASTO (Débito)
            double amountToReduce = std::abs(out.amount);
            
            // Procuramos nos UTXOs existentes do endereço para subtrair/remover
            for (auto it = utxos.begin(); it != utxos.end(); ) {
                if (it->address == out.address) {
                    if (it->amount <= amountToReduce) {
                        // O UTXO atual é menor ou igual ao que precisamos tirar
                        amountToReduce -= it->amount;
                        it = utxos.erase(it); // Remove o UTXO completamente
                    } else {
                        // O UTXO tem mais do que precisamos, apenas subtraímos
                        it->amount -= amountToReduce;
                        amountToReduce = 0;
                        break;
                    }
                } else {
                    ++it;
                }
                if (amountToReduce <= 0) break;
            }
        }
    }
}

// Retorna o saldo total de um endereço
double UTXOSet::getBalance(std::string address) {
    double balance = 0;
    for (const auto& u : utxos) {
        if (u.address == address) {
            balance += u.amount;
        }
    }
    return balance;
}

// Salva o estado dos UTXOs em binário
void UTXOSet::saveToFile(std::string filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) return;

    size_t size = utxos.size();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    
    for (const auto& u : utxos) {
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

// Carrega o estado dos UTXOs do disco
void UTXOSet::loadFromFile(std::string filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return;

    utxos.clear();
    size_t size;
    if(!file.read(reinterpret_cast<char*>(&size), sizeof(size))) return;

    for (size_t i = 0; i < size; ++i) {
        UTXO u;
        size_t txidLen, addrLen;

        file.read(reinterpret_cast<char*>(&txidLen), sizeof(txidLen));
        std::vector<char> txidBuf(txidLen + 1);
        file.read(txidBuf.data(), txidLen);
        txidBuf[txidLen] = '\0';
        u.txid = std::string(txidBuf.data());

        file.read(reinterpret_cast<char*>(&addrLen), sizeof(addrLen));
        std::vector<char> addrBuf(addrLen + 1);
        file.read(addrBuf.data(), addrLen);
        addrBuf[addrLen] = '\0';
        u.address = std::string(addrBuf.data());

        file.read(reinterpret_cast<char*>(&u.vout_index), sizeof(u.vout_index));
        file.read(reinterpret_cast<char*>(&u.amount), sizeof(u.amount));
        utxos.push_back(u);
    }
    file.close();
}
