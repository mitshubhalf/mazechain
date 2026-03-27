#include "../include/blockchain.h"
#include <iostream>
#include <fstream>
#include <sstream>

Blockchain::Blockchain() {
    difficulty = 2;
    totalSupply = 0;
    loadChain();
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

void Blockchain::mineBlock(std::string minerAddress) {
    int height = chain.size();
    int reward = getBlockReward(height);

    Transaction rewardTx = {"SYSTEM", minerAddress, (double)reward, ""};

    std::vector<Transaction> txs;
    txs.push_back(rewardTx);

    Block newBlock(height, txs, chain.back().hash);

    std::cout << "Minerando...\n";
    newBlock.mineBlock(difficulty);

    chain.push_back(newBlock);
    totalSupply += reward;

    saveChain();

    std::cout << "Bloco minerado: " << height << "\n";
}

double Blockchain::getBalance(std::string address) {
    double balance = 0;

    for (auto &block : chain) {
        for (auto &tx : block.transactions) {
            if (tx.to == address) balance += tx.amount;
            if (tx.from == address) balance -= tx.amount;
        }
    }

    return balance;
}

void Blockchain::saveChain() {
    std::ofstream file("chain.txt");

    for (auto &block : chain) {
        file << block.index << ";" << block.prevHash << ";" << block.hash << "\n";
    }

    file.close();
}

void Blockchain::loadChain() {
    std::ifstream file("chain.txt");

    if (!file.is_open()) {
        chain.push_back(Block(0, {}, "0"));
        saveChain();
        return;
    }

    chain.clear();

    std::string line;
    while (getline(file, line)) {
        std::stringstream ss(line);
        std::string i, prev, hash;

        getline(ss, i, ';');
        getline(ss, prev, ';');
        getline(ss, hash, ';');

        Block b(std::stoi(i), {}, prev);
        b.hash = hash;

        chain.push_back(b);
    }

    file.close();
}
