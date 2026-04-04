#include "../include/storage.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace Storage {

void saveChain(const Blockchain &bc, const std::string& filename) {
    std::ofstream file(filename);

    for (auto &block : bc.getChain()) {
        file << block.index << "|"
             << block.timestamp << "|"
             << block.hash << "|"
             << block.previousHash << "|"
             << block.nonce << "\n";
    }
}

void loadChain(Blockchain &bc, const std::string& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) return;

    bc.clearChain(); // 🔥 importante

    std::string line;
    while (getline(file, line)) {

        if (line.empty()) continue;

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

            bc.addLoadedBlock(b); // 🔥 NÃO minerar de novo
        } catch (...) {
            std::cout << "⚠️ Linha ignorada\n";
        }
    }
}

}
