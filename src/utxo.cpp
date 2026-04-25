#include "../include/utxo.h"
#include "../include/transaction.h"
#include <fstream>
#include <iostream>
#include <cmath>

// Atualizado para receber blockHeight para controle de maturidade
void UTXOSet::update(const Transaction& tx, int blockHeight) {
    // 1. Crédito: Adiciona novas saídas ao mapa
    for (size_t i = 0; i < tx.vout.size(); ++i) {
        const auto& out = tx.vout[i];

        // Verificação rigorosa do valor para evitar "dust" ou valores negativos estranhos
        if (out.amount > 0.000000001) {
            // A chave é txid + indice da saída
            std::string key = tx.id + ":" + std::to_string(i);

            // Verificação de segurança: Se o UTXO já existe, não somamos de novo no saldo global
            if (utxoMap.find(key) == utxoMap.end()) {
                // Identifica se é uma recompensa de mineração (Coinbase)
                bool isCoinbase = (tx.signature == "coinbase");

                // Criamos o novo UTXO com os dados de altura e tipo
                UTXO newUtxo;
                newUtxo.txid = tx.id;
                newUtxo.vout_index = (int)i;
                newUtxo.address = out.address;
                newUtxo.amount = out.amount;
                newUtxo.isCoinbase = isCoinbase;
                newUtxo.blockHeight = blockHeight;

                utxoMap[key] = newUtxo;

                // Atualiza cache de saldo global (saldo total, incluindo imaturos)
                addressBalances[out.address] += out.amount;
            }
        }
    }

    // 2. Débito: Remove UTXOs gastos
    for (const auto& in : tx.vin) {
        // Usando 'in.txid' e 'in.index' conforme seu transaction.h
        std::string key = in.txid + ":" + std::to_string(in.index);

        if (utxoMap.count(key)) {
            // Subtrai do cache de saldo antes de apagar
            addressBalances[utxoMap[key].address] -= utxoMap[key].amount;

            // Proteção para não deixar saldo negativo por erro de precisão float
            if (addressBalances[utxoMap[key].address] < 0) {
                addressBalances[utxoMap[key].address] = 0;
            }

            // Remove a moeda do conjunto (ela foi gasta)
            utxoMap.erase(key);
        }
    }
}

// getBalance atualizado para considerar a regra de maturidade de 50 blocos
double UTXOSet::getBalance(const std::string& address, int currentHeight) {
    double spendableBalance = 0.0;
    const int MATURITY_THRESHOLD = 50;

    // Percorremos os UTXOs para somar apenas o que pode ser gasto agora
    for (auto const& [key, u] : utxoMap) {
        if (u.address == address) {
            if (u.isCoinbase) {
                // Se for moeda de mineração, só libera após 50 blocos
                if (currentHeight - u.blockHeight >= MATURITY_THRESHOLD) {
                    spendableBalance += u.amount;
                }
            } else {
                // Transações normais entre usuários não têm bloqueio
                spendableBalance += u.amount;
            }
        }
    }

    return (spendableBalance < 0.000000001) ? 0.0 : spendableBalance;
}

void UTXOSet::saveToFile(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) return;

    size_t size = utxoMap.size();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));

    for (const auto& pair : utxoMap) {
        const UTXO& u = pair.second;

        // Salva TXID
        size_t txidLen = u.txid.length();
        file.write(reinterpret_cast<const char*>(&txidLen), sizeof(txidLen));
        file.write(u.txid.c_str(), txidLen);

        // Salva Endereço
        size_t addrLen = u.address.length();
        file.write(reinterpret_cast<const char*>(&addrLen), sizeof(addrLen));
        file.write(u.address.c_str(), addrLen);

        // Salva Indice e Valor
        file.write(reinterpret_cast<const char*>(&u.vout_index), sizeof(u.vout_index));
        file.write(reinterpret_cast<const char*>(&u.amount), sizeof(u.amount));

        // NOVOS CAMPOS: Salva flags de Coinbase e Altura do Bloco
        file.write(reinterpret_cast<const char*>(&u.isCoinbase), sizeof(u.isCoinbase));
        file.write(reinterpret_cast<const char*>(&u.blockHeight), sizeof(u.blockHeight));
    }
    file.close();
}

void UTXOSet::loadFromFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return;

    // Limpeza profunda antes de carregar
    utxoMap.clear();
    addressBalances.clear();

    size_t size;
    if(!file.read(reinterpret_cast<char*>(&size), sizeof(size))) return;

    for (size_t i = 0; i < size; ++i) {
        UTXO u;
        size_t txidLen, addrLen;

        // Lê TXID
        if(!file.read(reinterpret_cast<char*>(&txidLen), sizeof(txidLen))) break;
        u.txid.resize(txidLen);
        file.read(&u.txid[0], txidLen);

        // Lê Endereço
        if(!file.read(reinterpret_cast<char*>(&addrLen), sizeof(addrLen))) break;
        u.address.resize(addrLen);
        file.read(&u.address[0], addrLen);

        // Lê Indice e Valor
        file.read(reinterpret_cast<char*>(&u.vout_index), sizeof(u.vout_index));
        file.read(reinterpret_cast<char*>(&u.amount), sizeof(u.amount));

        // NOVOS CAMPOS: Lê flags de Coinbase e Altura do Bloco
        file.read(reinterpret_cast<char*>(&u.isCoinbase), sizeof(u.isCoinbase));
        file.read(reinterpret_cast<char*>(&u.blockHeight), sizeof(u.blockHeight));

        // Reconstrói os mapas
        std::string key = u.txid + ":" + std::to_string(u.vout_index);
        utxoMap[key] = u;
        addressBalances[u.address] += u.amount;
    }
    file.close();
}