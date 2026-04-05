#include "../include/blockchain.h"

// ==========================
// CONSTRUTOR
// ==========================
Blockchain::Blockchain() {
    chain.push_back(Block(0, {}, "0"));
}

// ==========================
// GET ÚLTIMO BLOCO
// ==========================
Block Blockchain::getLatestBlock() const {
    return chain.back();
}

// ==========================
// ADICIONAR BLOCO
// ==========================
void Blockchain::addBlock(Block newBlock) {
    newBlock.previousHash = getLatestBlock().hash;
    newBlock.mineBlock(4);
    chain.push_back(newBlock);
}

// ==========================
// GET CHAIN (CORRIGIDO)
// ==========================
const std::vector<Block>& Blockchain::getChain() const {
    return chain;
}

// ==========================
// LIMPAR CHAIN
// ==========================
void Blockchain::clearChain() {
    chain.clear();
}

// ==========================
// ADD BLOCO CARREGADO
// ==========================
void Blockchain::addLoadedBlock(const Block& block) {
    chain.push_back(block);
}

// ==========================
// ADICIONAR TRANSAÇÃO
// ==========================
void Blockchain::addTransaction(const Transaction& tx) {
    pendingTransactions.push_back(tx);
}

// ==========================
// MINERAR COM TRANSAÇÕES
// ==========================
void Blockchain::minePendingTransactions(const std::string& minerAddress) {

    // recompensa do minerador
    Transaction reward("SYSTEM", minerAddress, 50);
    pendingTransactions.push_back(reward);

    Block newBlock(chain.size(), pendingTransactions, getLatestBlock().hash);

    newBlock.mineBlock(4);

    chain.push_back(newBlock);

    // limpa mempool
    pendingTransactions.clear();
}

// ==========================
// CALCULAR SALDO
// ==========================
double Blockchain::getBalance(const std::string& address) {

    double balance = 0;

    for (const auto& block : chain) {
        for (const auto& tx : block.transactions) {

            if (tx.from == address)
                balance -= tx.amount;

            if (tx.to == address)
                balance += tx.amount;
        }
    }

    return balance;
}
