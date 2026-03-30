#include "../include/blockchain.h"
#include <fstream>
#include <sstream>

void saveChain(const Blockchain &bc) {
    std::ofstream file("chain.txt");

    for (auto &block : bc.chain) {
        file << block.index << "|"
             << block.timestamp << "|"
             << block.hash << "|"
             << block.previousHash << "|"
             << block.nonce << "\n";
    }
}

void loadChain(Blockchain &bc) {
    std::ifstream file("chain.txt");

    if (!file.is_open()) return;

    bc.chain.clear();

    std::string line;
    while (getline(file, line)) {
        std::stringstream ss(line);

        std::string index, timestamp, hash, prev, nonce;

        getline(ss, index, '|');
        getline(ss, timestamp, '|');
        getline(ss, hash, '|');
        getline(ss, prev, '|');
        getline(ss, nonce, '|');

        Block b(std::stoi(index), {}, prev);
        b.timestamp = timestamp;
        b.hash = hash;
        b.nonce = std::stoi(nonce);

        bc.chain.push_back(b);
    }
}
