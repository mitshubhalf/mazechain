#include "../include/utxo.h"
#include "../include/transaction.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <iomanip>

// Constante para comparação de valores decimais (1 Mit)
const double EPSILON = 0.000000001;

void UTXOSet::update(const Transaction& tx) {
    for (const auto& out : tx.vout) {
        if (out.amount > 0) {
            // CRÉDITO: Adiciona novo UTXO
            UTXO newUtxo;
            newUtxo.txid = tx.id;
            newUtxo.address = out.address;
            newUtxo.amount = out.amount;
            newUtxo.vout_index = 0; 
            utxos.push_back(newUtxo);
        } 
        else if (out.amount < 0) {
            // DÉBITO: Remove saldo do endereço
            double amountToReduce = std::abs(out.amount);
            
            for (auto it = utxos.begin(); it != utxos.end(); ) {
                if (it->address == out.address) {
                    // Se o UTXO é menor que o necessário (ou igual, considerando erro de precisão)
                    if (it->amount <= (amountToReduce + EPSILON)) {
                        amountToReduce -= it->amount;
                        it = utxos.erase(it);
                    } else {
                        // O UTXO tem saldo sobrando, apenas subtrai
                        it->amount -= amountToReduce;
                        amountToReduce = 0;
                        break;
                    }
                } else {
                    ++it;
                }
                if (amountToReduce < EPSILON) break;
            }
        }
    }
}

double UTXOSet::getBalance(std::string address) {
    double balance = 0;
    for (const auto& u : utxos) {
        if (u.address == address) {
            balance += u.amount;
        }
    }
    // Retorna o saldo limpo, evitando lixo de memória decimal
    return (balance < EPSILON) ? 0.0 : balance;
}

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

void UTXOSet::loadFromFile(std::string filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return;

    utxos.clear();
    size_t size;
    if(!file.read(reinterpret_cast<char*>(&size), sizeof(size))) return;

    for (size_t i = 0; i < size; ++i) {
        UTXO u;
        size_t txidLen, addrLen;

        if(!file.read(reinterpret_cast<char*>(&txidLen), sizeof(txidLen))) break;
        u.txid.resize(txidLen);
        file.read(&u.txid[0], txidLen);

        if(!file.read(reinterpret_cast<char*>(&addrLen), sizeof(addrLen))) break;
        u.address.resize(addrLen);
        file.read(&u.address[0], addrLen);

        file.read(reinterpret_cast<char*>(&u.vout_index), sizeof(u.vout_index));
        file.read(reinterpret_cast<char*>(&u.amount), sizeof(u.amount));
        utxos.push_back(u);
    }
    file.close();
}
