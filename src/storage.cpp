#include "../include/storage.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace Storage {

void saveChain(const Blockchain &bc, const std::string& filename) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cout << "Erro ao salvar blockchain\n";
        return;
    }

    const auto& chain = bc.getChain();

    for (const auto& block : chain) {
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

    bc.clearChain();

    std::string line;

    while (std::getline(file, line)) {

        if (line.empty()) continue;

        std::stringstream ss(line);

        std::string indexStr, timestamp, hash, prevHash, nonceStr;

        if (!std::getline(ss, indexStr, '|')) continue;
        if (!std::getline(ss, timestamp, '|')) continue;
        if (!std::getline(ss, hash, '|')) continue;
        if (!std::getline(ss, prevHash, '|')) continue;
        if (!std::getline(ss, nonceStr, '|')) continue;

        try {
            int index = std::stoi(indexStr);
            int nonce = std::stoi(nonceStr);

            Block b(index, {}, prevHash);
            b.timestamp = timestamp;
            b.hash = hash;
            b.nonce = nonce;

            bc.addLoadedBlock(b);

        } catch (...) {
            std::cout << "Erro ao carregar bloco\n";
        }
    }
}

}
