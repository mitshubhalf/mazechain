#include "../include/blockchain.h"
#include "../include/crypto.h"
#include <iostream>

Blockchain::Blockchain() {
    difficulty = 2;
    totalSupply = 0;

    chain.push_back(Block(0, {}, "0"));
}

int Blockchain::getBlockReward(int height) {
    int reward = 250;
    int interval = 1000;

    while (height >= interval && reward > 1) {
        reward /= 2;
        interval *= 2;
    }

    return reward;
}

bool Blockchain::addTransaction(Transaction tx) {
    if (tx.from == "SYSTEM") {
        mempool.push_back(tx);
        return true;
    }

    if (getBalance(tx.from) < tx.amount) {
        std::cout << "Saldo insuficiente\n";
        return false;
    }

    if (!verifySignature(tx.toString(), tx.signature, tx.from)) {
        std::cout << "Assinatura inválida\n";
        return false;
    }

    mempool.push_back(tx);
    return true;
}

void Blockchain::mineBlock(std::string minerAddress) {
    if (totalSupply >= 20000000) {
        std::cout << "Supply máximo atingido\n";
        return;
    }

    int height = chain.size();
    int reward = getBlockReward(height);

    Transaction rewardTx = {"SYSTEM", minerAddress, (double)reward, ""};

    std::vector<Transaction> blockTxs = mempool;
    blockTxs.push_back(rewardTx);

    Block newBlock(height, blockTxs, chain.back().hash);

    std::cout << "Minerando...\n";
    newBlock.mineBlock(difficulty);

    chain.push_back(newBlock);
    totalSupply += reward;

    mempool.clear();

    std::cout << "Bloco minerado: " << height << "\n";
}
double Blockchain::getBalance(std::string address) {
    double balance = 0;

    for (auto &block : chain) {
        for (auto &tx : block.transactions) {
            if (tx.to == address)
                balance += tx.amount;
            if (tx.from == address)
                balance -= tx.amount;
        }
    }

    return balance;
}

double Blockchain::getTotalSupply() {
    return totalSupply;
}
