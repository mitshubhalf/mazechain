#include "../include/storage.h"
#include <fstream>
#include <sstream>
#include <iostream>

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

        if (line.empty()) continue; // 🔥 evita erro

        std::stringstream ss(line);

        std::string index, timestamp, hash, prev, nonce;

        if (!getline(ss, index, '|')) continue;
        if (!getline(ss, timestamp, '|')) continue;
        if (!getline(ss, hash, '|')) continue;
        if (!getline(ss, prev, '|')) continue;
        if (!getline(ss, nonce, '|')) continue;

        try {
            Block b(std::stoi(index), {}, prev);
            b.timestamp = timestamp;
            b.hash = hash;
            b.nonce = std::stoi(nonce);

            bc.chain.push_back(b);
        } catch (...) {
            std::cout << "⚠️ Linha ignorada (erro de parsing)\n";
            continue;
        }
    }
}
