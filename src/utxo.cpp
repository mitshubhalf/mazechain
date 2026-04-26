#include "../include/utxo.h"
#include "../include/transaction.h"
#include <fstream>
#include <iostream>
#include <cmath>

// Atualizado para receber blockHeight para controle de maturidade
void UTXOSet::update(const Transaction& tx, int blockHeight) {
    // 1. Crédito (e Débito via valor negativo no VOUT):
    for (size_t i = 0; i < tx.vout.size(); ++i) {
        const auto& out = tx.vout[i];

        // Se o valor for negativo (Lógica de Débito do seu Blockchain::send)
        if (out.amount < -0.000000001) { 
            addressBalances[out.address] += out.amount; // Soma valor negativo = Subtração

            // --- CORREÇÃO CRÍTICA ---
            // No modelo UTXO, um valor negativo no VOUT significa que precisamos 
            // "anular" moedas existentes ou criar um débito persistente.
            // Para manter seu sistema, criamos um UTXO de débito para o getBalance enxergar.
            std::string key = tx.id + ":" + std::to_string(i) + "_DEBIT";
            UTXO debitUtxo;
            debitUtxo.txid = tx.id;
            debitUtxo.vout_index = (int)i;
            debitUtxo.address = out.address;
            debitUtxo.amount = out.amount; // Valor negativo
            debitUtxo.isCoinbase = false;  // Débito não é coinbase, é imediato
            debitUtxo.blockHeight = blockHeight;

            utxoMap[key] = debitUtxo;

            if (addressBalances[out.address] < 0.000000001) {
                addressBalances[out.address] = 0;
            }
            continue; 
        }

        // Verificação rigorosa do valor para evitar "dust"
        if (out.amount > 0.000000001) {
            std::string key = tx.id + ":" + std::to_string(i);

            if (utxoMap.find(key) == utxoMap.end()) {
                bool isCoinbase = (tx.signature == "coinbase");

                UTXO newUtxo;
                newUtxo.txid = tx.id;
                newUtxo.vout_index = (int)i;
                newUtxo.address = out.address;
                newUtxo.amount = out.amount;
                newUtxo.isCoinbase = isCoinbase;
                newUtxo.blockHeight = blockHeight;

                utxoMap[key] = newUtxo;
                addressBalances[out.address] += out.amount;
            }
        }
    }

    // 2. Débito Tradicional via VIN:
    for (const auto& in : tx.vin) {
        std::string key = in.txid + ":" + std::to_string(in.index);

        if (utxoMap.count(key)) {
            addressBalances[utxoMap[key].address] -= utxoMap[key].amount;

            if (addressBalances[utxoMap[key].address] < 0.000000001) {
                addressBalances[utxoMap[key].address] = 0;
            }

            utxoMap.erase(key);
        }
    }
}

// getBalance atualizado para considerar a regra de maturidade e débitos negativos
double UTXOSet::getBalance(const std::string& address, int currentHeight) {
    double spendableBalance = 0.0;
    const int MATURITY_THRESHOLD = 50;

    for (auto const& [key, u] : utxoMap) {
        if (u.address == address) {
            if (u.isCoinbase) {
                // Moeda de mineração: maturidade de 50 blocos
                if (currentHeight - u.blockHeight >= MATURITY_THRESHOLD) {
                    spendableBalance += u.amount;
                }
            } else {
                // Transações normais e DÉBITOS NEGATIVOS entram aqui
                // Como débitos têm amount negativo, eles reduzem o spendableBalance
                spendableBalance += u.amount;
            }
        }
    }

    return (spendableBalance < 0.000000001) ? 0.0 : spendableBalance;
}

void UTXOSet::saveToFile(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file) return;

    size_t size = utxoMap.size();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));

    for (const auto& pair : utxoMap) {
        const UTXO& u = pair.second;

        size_t txidLen = u.txid.length();
        file.write(reinterpret_cast<const char*>(&txidLen), sizeof(txidLen));
        file.write(u.txid.c_str(), txidLen);

        size_t addrLen = u.address.length();
        file.write(reinterpret_cast<const char*>(&addrLen), sizeof(addrLen));
        file.write(u.address.c_str(), addrLen);

        file.write(reinterpret_cast<const char*>(&u.vout_index), sizeof(u.vout_index));
        file.write(reinterpret_cast<const char*>(&u.amount), sizeof(u.amount));
        file.write(reinterpret_cast<const char*>(&u.isCoinbase), sizeof(u.isCoinbase));
        file.write(reinterpret_cast<const char*>(&u.blockHeight), sizeof(u.blockHeight));
    }
    file.close();
}

void UTXOSet::loadFromFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return;

    utxoMap.clear();
    addressBalances.clear();

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
        file.read(reinterpret_cast<char*>(&u.isCoinbase), sizeof(u.isCoinbase));
        file.read(reinterpret_cast<char*>(&u.blockHeight), sizeof(u.blockHeight));

        // Reconstrói a chave lidando com possíveis débitos negativos salvos
        std::string key = u.txid + ":" + std::to_string(u.vout_index);
        if (u.amount < 0) key += "_DEBIT";

        utxoMap[key] = u;
        addressBalances[u.address] += u.amount;
    }
    file.close();
}