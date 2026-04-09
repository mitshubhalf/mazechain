#include "../include/storage.h"
#include <fstream>
#include <iostream>

void Storage::saveChain(const Blockchain& bc, const std::string& filename) {

    std::ofstream file(filename);

    if (!file.is_open()) return;

    for (const auto& block : bc.getChain()) {
        file << block.index << "|"
             << block.hash << "|"
             << block.previousHash << "\n";
    }

    file.close();
}

void Storage::loadChain(Blockchain& bc, const std::string& filename) {

    std::ifstream file(filename);

    if (!file.is_open()) return;

    std::string line;

    bc = Blockchain(); // reset

    while (std::getline(file, line)) {

        int index;
        std::string hash, prevHash;

        std::stringstream ss(line);

        std::getline(ss, line, '|');
        index = std::stoi(line);

        std::getline(ss, hash, '|');
        std::getline(ss, prevHash);

        Block b(index, {}, prevHash);

        b.hash = hash; // 🔥 IMPORTANTE: não recalcular

        bc.getChain().push_back(b); // ⚠️ precisa ajustar header se der erro
    }

    file.close();
}
