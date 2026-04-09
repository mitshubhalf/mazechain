#include "../include/storage.h"
#include <fstream>
#include <iostream>

void Storage::saveChain(const Blockchain& bc, const std::string& filename) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cout << "Erro ao salvar blockchain\n";
        return;
    }

    for (const auto& block : bc.chain) {
        file << "BLOCK\n";
        file << block.index << "\n";
        file << block.previousHash << "\n";
        file << block.hash << "\n";
        file << block.nonce << "\n";

        for (const auto& tx : block.transactions) {
            file << "TX\n";
            file << tx.from << "\n";
            file << tx.to << "\n";
            file << tx.amount << "\n";
        }

        file << "END_BLOCK\n";
    }

    file.close();
}

void Storage::loadChain(Blockchain& bc, const std::string& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) return;

    bc.chain.clear();

    std::string line;
    Block* currentBlock = nullptr;

    while (std::getline(file, line)) {
        if (line == "BLOCK") {
            int index;
            std::string prevHash, hash;
            int nonce;

            file >> index;
            file.ignore();

            std::getline(file, prevHash);
            std::getline(file, hash);

            file >> nonce;
            file.ignore();

            currentBlock = new Block(index, prevHash, {});
            currentBlock->hash = hash;
            currentBlock->nonce = nonce;
        }

        else if (line == "TX") {
            std::string from, to;
            double amount;

            std::getline(file, from);
            std::getline(file, to);
            file >> amount;
            file.ignore();

            Transaction tx(from, to, amount);
            currentBlock->transactions.push_back(tx);
        }

        else if (line == "END_BLOCK") {
            bc.chain.push_back(*currentBlock);
            delete currentBlock;
        }
    }

    file.close();
}
