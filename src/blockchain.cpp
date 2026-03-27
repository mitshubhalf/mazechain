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
            if (tx.to == address)
                balance += tx.amount;
            if (tx.from == address)
                balance -= tx.amount;
        }
    }

    return balance;
}

void Blockchain::saveChain() {
    std::ofstream file("chain.txt");

    for (auto &block : chain) {
        file << block.index << "|"
             << block.prevHash << "|"
             << block.hash << "|"
             << block.transactions.size();

        for (auto &tx : block.transactions) {
            file << "|" << tx.from << "," << tx.to << "," << tx.amount;
        }

        file << "\n";
    }

    file.close();
}

void Blockchain::loadChain() {
    std::ifstream file("chain.txt");

    if (!file.is_open()) {
        std::cout << "Criando genesis block...\n";
        chain.clear();
        chain.push_back(Block(0, {}, "0"));
        saveChain();
        return;
    }

    chain.clear();

    std::string line;
    while (getline(file, line)) {
        std::stringstream ss(line);
        std::string part;

        std::vector<std::string> parts;

        while (getline(ss, part, '|')) {
            parts.push_back(part);
        }

        int index = std::stoi(parts[0]);
        std::string prev = parts[1];
        std::string hash = parts[2];
        int txCount = std::stoi(parts[3]);

        std::vector<Transaction> txs;

        for (int i = 0; i < txCount; i++) {
            std::stringstream txStream(parts[4 + i]);
            std::string from, to, amount;

            getline(txStream, from, ',');
            getline(txStream, to, ',');
            getline(txStream, amount, ',');

            txs.push_back({from, to, std::stod(amount), ""});
        }

        Block block(index, txs, prev);
        block.hash = hash;

        chain.push_back(block);
    }

    file.close();
}
