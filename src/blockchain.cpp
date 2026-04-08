#include "../include/blockchain.h"
#include <iostream>

// =====================
// UTXO STRUCT
// =====================
struct UTXO {
    std::string txId;
    int index;
    std::string address;
    double amount;
};

// UTXO POOL
std::vector<UTXO> utxoPool;

// =====================
// CONSTRUTOR
// =====================
Blockchain::Blockchain() {
    chain.push_back(Block(0, {}, "0"));
}

// =====================
// GET LAST BLOCK
// =====================
Block Blockchain::getLatestBlock() const {
    return chain.back();
}

// =====================
// ADD BLOCK
// =====================
void Blockchain::addBlock(Block newBlock) {
    newBlock.previousHash = getLatestBlock().hash;
    newBlock.mineBlock(4);
    chain.push_back(newBlock);

    // atualizar UTXOs
    for (const auto& tx : newBlock.transactions) {

        // remover inputs consumidos
        for (const auto& input : tx.inputs) {
            utxoPool.erase(
                std::remove_if(utxoPool.begin(), utxoPool.end(),
                    [&](const UTXO& u) {
                        return u.txId == input.txId && u.index == input.outputIndex;
                    }),
                utxoPool.end()
            );
        }

        // adicionar novos outputs
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

// =====================
// BALANCE (UTXO)
// =====================
double Blockchain::getBalance(const std::string& address) const {

    double balance = 0;

    for (const auto& utxo : utxoPool) {
        if (utxo.address == address) {
            balance += utxo.amount;
        }
    }

    return balance;
}

// =====================
// VALIDATE CHAIN
// =====================
bool Blockchain::isChainValid() const {

    for (size_t i = 1; i < chain.size(); i++) {

        const Block& current = chain[i];
        const Block& previous = chain[i - 1];

        if (current.hash != current.calculateHash())
            return false;

        if (current.previousHash != previous.hash)
            return false;
    }

    return true;
}

// =====================
// PENDING TRANSACTIONS
// =====================
void Blockchain::addTransaction(const Transaction& tx) {
    pendingTransactions.push_back(tx);
}

// =====================
// MINING
// =====================
void Blockchain::minePendingTransactions(const std::string& minerAddress) {

    // reward
    Transaction reward;
    reward.id = "reward";

    TxOutput out;
    out.address = minerAddress;
    out.amount = 50;

    reward.outputs.push_back(out);

    pendingTransactions.push_back(reward);

    Block newBlock(chain.size(), pendingTransactions, getLatestBlock().hash);

    newBlock.mineBlock(4);

    chain.push_back(newBlock);

    pendingTransactions.clear();
}

// =====================
// GET PENDING
// =====================
const std::vector<Transaction>& Blockchain::getPendingTransactions() const {
    return pendingTransactions;
}
