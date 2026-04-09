#include "../include/storage.h"
#include <fstream>
#include <sstream>

void Storage::saveChain(const Blockchain& bc, const std::string& filename) {
    std::ofstream file(filename);

    if (!file.is_open()) return;

    for (const auto& block : bc.getChain()) {
        file << block.index << "|"
             << block.hash << "|"
             << block.previousHash << "\n";

        for (const auto& tx : block.transactions) {
            file << tx.id << "\n";

            for (const auto& out : tx.outputs) {
                file << out.address << "|" << out.amount << "\n";
            }

            file << "#\n";
        }

        file << "---\n";
    }

    file.close();
}

void Storage::loadChain(Blockchain& bc, const std::string& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) return;

    std::string line;
    std::vector<Block> loadedChain;

    while (std::getline(file, line)) {

        if (line.empty()) continue;

        std::stringstream ss(line);

        std::string indexStr, hash, prevHash;

        std::getline(ss, indexStr, '|');
        std::getline(ss, hash, '|');
        std::getline(ss, prevHash);

        int index = std::stoi(indexStr);

        std::vector<Transaction> txs;

        while (std::getline(file, line) && line != "---") {

            if (line.empty()) continue;

            Transaction tx;
            tx.id = line;

            while (std::getline(file, line) && line != "#") {
                std::stringstream ss2(line);

                std::string addr, amountStr;

                std::getline(ss2, addr, '|');
                std::getline(ss2, amountStr);

                TxOutput out;
                out.address = addr;
                out.amount = std::stod(amountStr);

                tx.outputs.push_back(out);
            }

            txs.push_back(tx);
        }

        Block b(index, txs, prevHash);
        b.hash = hash;

        loadedChain.push_back(b);
    }

    file.close();

    bc.replaceChain(loadedChain);
}
