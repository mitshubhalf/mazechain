#include "../include/blockchain.h"
#include "../include/blockchain.h"

// CONSTRUTOR
Blockchain::Blockchain() {
    chain.push_back(Block(0, {}, "0"));
}

// ÚLTIMO BLOCO
Block Blockchain::getLatestBlock() const {
    return chain.back();
}

// ADD BLOCO
void Blockchain::addBlock(Block newBlock) {
    newBlock.previousHash = getLatestBlock().hash;
    newBlock.mineBlock(4);
    chain.push_back(newBlock);
}

// GET CHAIN
std::vector<Block>& Blockchain::getChain() {
    return chain;
}

const std::vector<Block>& Blockchain::getChain() const {
    return chain;
}

// LIMPAR
void Blockchain::clearChain() {
    chain.clear();
}

// ADD BLOCO CARREGADO
void Blockchain::addLoadedBlock(const Block& block) {
    chain.push_back(block);
}

// ADD TRANSAÇÃO
void Blockchain::addTransaction(const Transaction& tx) {
    pendingTransactions.push_back(tx);
}

// ⛏️ MINERAR
void Blockchain::minePendingTransactions(const std::string& minerAddress) {

    // reward
    Transaction reward("", minerAddress, 50);
    pendingTransactions.push_back(reward);

    Block newBlock(chain.size(), pendingTransactions, getLatestBlock().hash);

    newBlock.mineBlock(4);

    chain.push_back(newBlock);

    pendingTransactions.clear();
}

// 💰 SALDO
double Blockchain::getBalance(const std::string& address) const {

    double balance = 0;

    for (const auto& block : chain) {
        for (const auto& tx : block.transactions) {

            if (!tx.from.empty() && tx.from == address)
                balance -= tx.amount;

            if (!tx.to.empty() && tx.to == address)
                balance += tx.amount;
        }
    }

    return balance;
}

// 🔒 VALIDAÇÃO DA CHAIN
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
// CONSTRUTOR
Blockchain::Blockchain() {
    chain.push_back(Block(0, {}, "0"));
}

// ÚLTIMO BLOCO
Block Blockchain::getLatestBlock() const {
    return chain.back();
}

// GET CHAIN
std::vector<Block>& Blockchain::getChain() {
    return chain;
}

const std::vector<Block>& Blockchain::getChain() const {
    return chain;
}

// LIMPAR
void Blockchain::clearChain() {
    chain.clear();
}

// ADD BLOCO CARREGADO
void Blockchain::addLoadedBlock(const Block& block) {
    chain.push_back(block);
}

// ADD TRANSAÇÃO
void Blockchain::addTransaction(const Transaction& tx) {
    pendingTransactions.push_back(tx);
}

// ⛏️ MINERAR
void Blockchain::minePendingTransactions(const std::string& minerAddress) {

    Transaction reward("", minerAddress, 50);
    pendingTransactions.push_back(reward);

    Block newBlock(chain.size(), pendingTransactions, getLatestBlock().hash);

    newBlock.mineBlock(4);

    chain.push_back(newBlock);

    pendingTransactions.clear();
}

// 💰 SALDO
double Blockchain::getBalance(const std::string& address) const {
    double balance = 0;

    for (const auto& block : chain) {
        for (const auto& tx : block.transactions) {

            if (!tx.from.empty() && tx.from == address)
                balance -= tx.amount;

            if (!tx.to.empty() && tx.to == address)
                balance += tx.amount;
        }
    }

    return balance;
}
