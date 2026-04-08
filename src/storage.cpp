#include "../include/storage.h"
#include <fstream>
#include <iostream>

void Storage::saveChain(const Blockchain& bc, const std::string& filename) {
    std::ofstream file(filename);

    if (!file.is_open()) return;

    for (const auto& block : bc.getChain()) {
        file << block.hash << "|";
        file << block.previousHash << "\n";

        for (const auto& tx : block.transactions) {
            file << tx.id << "\n";
        }

        file << "---\n";
    }

    file.close();
}

void Storage::loadChain(Blockchain& bc, const std::string& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) return;

    std::string line;

    while (std::getline(file, line)) {
        if (line == "---") continue;

        // 🔧 CORREÇÃO: Block não tem construtor vazio
        int index = 0;
        std::vector<Transaction> txs;
        std::string prevHash = "0";

        Block b(index, txs, prevHash);

        bc.addBlock(b);
    }

    file.close();
}
