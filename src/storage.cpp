#include "../include/storage.h"
#include <fstream>
#include <sstream>
#include <iostream>

void Storage::saveChain(const Blockchain& bc, const std::string& filename) {
    std::ofstream file(filename);

    for (const auto& block : bc.getChain()) {
        file << block.index << "|"
             << block.hash << "|"
             << block.previousHash << "\n";

        for (const auto& tx : block.transactions) {
            file << "TX|" << tx.id << "\n";

            for (const auto& out : tx.outputs) {
                file << "OUT|" << out.address << "|" << out.amount << "\n";
            }
        }

        file << "---\n";
    }
}

void Storage::loadChain(Blockchain& bc, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    bc.clearChain();

    std::string line;
    Block* currentBlock = nullptr;

    while (std::getline(file, line)) {

        if (line == "---") {
            if (currentBlock) {
                bc.addBlockDirect(*currentBlock);
                delete currentBlock;
                currentBlock = nullptr;
            }
            continue;
        }

        std::stringstream ss(line);
        std::string type;
        std::getline(ss, type, '|');

        if (type != "TX" && type != "OUT") {
            int index = std::stoi(type);
            std::string hash, prev;

            std::getline(ss, hash, '|');
            std::getline(ss, prev, '|');

            currentBlock = new Block(index, {}, prev);
            currentBlock->hash = hash;
        }
        else if (type == "TX") {
            std::string id;
            std::getline(ss, id, '|');

            Transaction tx;
            tx.id = id;

            currentBlock->transactions.push_back(tx);
        }
        else if (type == "OUT") {
            std::string addr, amountStr;
            std::getline(ss, addr, '|');
            std::getline(ss, amountStr, '|');

            TxOutput out;
            out.address = addr;
            out.amount = std::stod(amountStr);

            currentBlock->transactions.back().outputs.push_back(out);
        }
    }
}
