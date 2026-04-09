#include "../include/blockchain.h"
#include <iostream>
#include <algorithm>

Blockchain::Blockchain() {
    chain.push_back(Block(0, {}, "0"));
}

Block Blockchain::getLatestBlock() const {
    return chain.back();
}

const std::vector<Block>& Blockchain::getChain() const {
    return chain;
}

void Blockchain::addBlock(Block newBlock) {

    std::cout << "📦 Adicionando novo bloco...\n";

    newBlock.previousHash = getLatestBlock().hash;

    int difficulty = 4;
    std::cout << "⚙️ Dificuldade: " << difficulty << "\n";

    newBlock.mineBlock(difficulty);

    chain.push_back(newBlock);

    std::cout << "✅ Bloco adicionado!\n";

    // 🔥 atualizar UTXO
    for (const auto& tx : newBlock.transactions) {

        // remover UTXOs usados
        for (const auto& input : tx.inputs) {
            utxoPool.erase(
                std::remove_if(utxoPool.begin(), utxoPool.end(),
                    [&](const UTXO& u) {
                        return u.txId == input.txId && u.index == input.outputIndex;
                    }),
                utxoPool.end()
            );
        }

        // adicionar novos UTXOs
        for (size_t i = 0; i < tx.outputs.size(); i++) {
            UTXO utxo;
            utxo.txId = tx.id;
            utxo.index = i;
            utxo.address = tx.outputs[i].address;
            utxo.amount = tx.outputs[i].amount;

            utxoPool.push_back(utxo);
        }
    }
}

void Blockchain::addTransaction(const Transaction& tx) {

    double inputTotal = 0;

    for (const auto& input : tx.inputs) {
        for (const auto& utxo : utxoPool) {
            if (utxo.txId == input.txId && utxo.index == input.outputIndex) {
                inputTotal += utxo.amount;
            }
        }
    }

    double outputTotal = 0;
    for (const auto& out : tx.outputs) {
        outputTotal += out.amount;
    }

    if (inputTotal < outputTotal) {
        std::cout << "❌ Saldo insuficiente!\n";
        return;
    }

    pendingTransactions.push_back(tx);

    std::cout << "✅ Transação adicionada\n";
}

void Blockchain::minePendingTransactions(const std::string& minerAddress) {

    std::cout << "⛏️ Iniciando mineração...\n";

    // 🔥 criar transação de recompensa
    Transaction reward;
    reward.id = "reward_" + minerAddress;

    TxOutput out;
    out.address = minerAddress;
    out.amount = 50;

    reward.outputs.push_back(out);

    pendingTransactions.push_back(reward);

    std::cout << "📦 Criando bloco com " << pendingTransactions.size() << " transações...\n";

    Block newBlock(chain.size(), pendingTransactions, getLatestBlock().hash);

    int difficulty = 4; // 👉 teste com 2 se quiser rápido
    std::cout << "⚙️ Dificuldade: " << difficulty << "\n";

    // 🔥 mineração com logs (vem do block.cpp)
    newBlock.mineBlock(difficulty);

    chain.push_back(newBlock);

    std::cout << "🎉 Bloco minerado e adicionado na chain!\n";

    // 🔥 atualizar UTXO (ESSENCIAL)
    for (const auto& tx : newBlock.transactions) {

        // remover inputs
        for (const auto& input : tx.inputs) {
            utxoPool.erase(
                std::remove_if(utxoPool.begin(), utxoPool.end(),
                    [&](const UTXO& u) {
                        return u.txId == input.txId && u.index == input.outputIndex;
                    }),
                utxoPool.end()
            );
        }

        // adicionar outputs
        for (size_t i = 0; i < tx.outputs.size(); i++) {
            UTXO utxo;
            utxo.txId = tx.id;
            utxo.index = i;
            utxo.address = tx.outputs[i].address;
            utxo.amount = tx.outputs[i].amount;

            utxoPool.push_back(utxo);
        }
    }

    pendingTransactions.clear();

    std::cout << "✅ Mineração finalizada!\n";
}

double Blockchain::getBalance(const std::string& address) const {

    double balance = 0;

    for (const auto& utxo : utxoPool) {
        if (utxo.address == address) {
            balance += utxo.amount;
        }
    }

    return balance;
}

// ✅ reconstrução do UTXO (importante para load/save)
void Blockchain::rebuildUTXO() {

    std::cout << "♻️ Reconstruindo UTXO...\n";

    utxoPool.clear();

    for (const auto& block : chain) {
        for (const auto& tx : block.transactions) {

            // remover inputs
            for (const auto& input : tx.inputs) {
                utxoPool.erase(
                    std::remove_if(utxoPool.begin(), utxoPool.end(),
                        [&](const UTXO& u) {
                            return u.txId == input.txId && u.index == input.outputIndex;
                        }),
                    utxoPool.end()
                );
            }

            // adicionar outputs
            for (size_t i = 0; i < tx.outputs.size(); i++) {
                UTXO utxo;
                utxo.txId = tx.id;
                utxo.index = i;
                utxo.address = tx.outputs[i].address;
                utxo.amount = tx.outputs[i].amount;

                utxoPool.push_back(utxo);
            }
        }
    }

    std::cout << "✅ UTXO reconstruído!\n";
}
