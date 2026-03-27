#include "../include/blockchain.h"
#include <fstream>
#include <sstream>
#include <iostream>

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

void Blockchain::mineBlock(std::string miner) {

    int height = chain.size();

    int reward = getBlockReward(height);

    Transaction rewardTx = {"SYSTEM", miner, (double)reward, ""};

    std::vector<Transaction> txs = mempool;
    txs.push_back(rewardTx);

    Block block(height, txs, chain.back().hash);

    std::cout << "Minerando...\n";
    block.mineBlock(difficulty);

    chain.push_back(block);

    totalSupply += reward;

    mempool.clear();

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

        file << block.index << ";"
             << block.prevHash << ";"
             << block.hash;

        // salvar transações
        for (auto &tx : block.transactions) {
            file << ";" << tx.from << "," << tx.to << "," << tx.amount;
        }

        file << "\n";
    }

    file.close();
}

void Blockchain::loadChain() {
    std::ifstream file("chain.txt");

    if (!file.is_open()) {
        std::cout << "Criando genesis block...\n";
        chain.push_back(Block(0, {}, "0"));
        saveChain();
        return;
    }

    chain.clear();

    std::string line;
    while (getline(file, line)) {

        std::stringstream ss(line);

        std::string index, prev, hash;
        getline(ss, index, ';');
        getline(ss, prev, ';');
        getline(ss, hash, ';');

        std::vector<Transaction> txs;

        std::string txData;
        while (getline(ss, txData, ';')) {

            std::stringstream txsStream(txData);

            std::string from, to, amount;

            getline(txsStream, from, ',');
            getline(txsStream, to, ',');
            getline(txsStream, amount, ',');

            txs.push_back({from, to, std::stod(amount), ""});
        }

        Block block(std::stoi(index), txs, prev);
        block.hash = hash;

        chain.push_back(block);
    }

    file.close();
}
