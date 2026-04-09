#include "../include/blockchain.h"
#include <iostream>

Blockchain::Blockchain() {
    difficulty = 2;
    totalSupply = 0;

    chain.push_back(Block(0, "0", {}));
}

Block Blockchain::getLastBlock() {
    return chain.back();
}

double Blockchain::getBlockReward(int height) {
    double reward = 250;
    int halvings = height / 1000;

    for (int i = 0; i < halvings; i++)
        reward /= 2;

    if (totalSupply >= 20000000)
        return 0;

    return reward;
}

void Blockchain::mineBlock(std::string minerAddress) {
    double reward = getBlockReward(chain.size());

    Transaction coinbase({}, { {minerAddress, reward} });

    Block newBlock(chain.size(), getLastBlock().hash, {coinbase});

    std::cout << "⛏️ Mining...\n";
    newBlock.mine(difficulty);

    chain.push_back(newBlock);
    totalSupply += reward;

    std::cout << "🎉 Reward: " << reward << "\n";
}

double Blockchain::getBalance(std::string address) {
    double balance = 0;

    for (auto &block : chain) {
        for (auto &tx : block.transactions) {
            for (auto &out : tx.vout) {
                if (out.address == address)
                    balance += out.amount;
            }
        }
    }

    return balance;
}

void Blockchain::send(std::string from, std::string to, double amount) {
    if (getBalance(from) < amount) {
        std::cout << "❌ Saldo insuficiente\n";
        return;
    }

    Transaction tx({}, { {to, amount} });

    Block newBlock(chain.size(), getLastBlock().hash, {tx});
    newBlock.mine(difficulty);

    chain.push_back(newBlock);

    std::cout << "✅ Transação enviada\n";
}
