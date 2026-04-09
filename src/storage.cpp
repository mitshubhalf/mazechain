#include "../include/storage.h"
#include <fstream>
#include <sstream>
#include <iostream>

void Storage::saveChain(const Blockchain& bc, const std::string& filename) {

    std::ofstream file(filename);
    if (!file.is_open()) return;

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

    file.close();
}

void Storage::loadChain(Blockchain& bc, const std::string& filename) {

    std::ifstream file(filename);
    if (!file.is_open()) return;

    std::string line;
    std::vector<Block> loadedBlocks;

    int index;
    std::string hash, prevHash;
    std::vector<Transaction> txs;
    Transaction currentTx;

    while (std::getline(file, line)) {

        if (line == "---") {
            Block b(index, txs, prevHash);
            b.hash = hash;
            loadedBlocks.push_back(b);

            txs.clear();
            continue;
        }

        std::stringstream ss(line);
        std::string type;
        getline(ss, type, '|');

        if (type != "TX" && type != "OUT") {
            std::stringstream ss2(line);
            std::string idxStr;

            getline(ss2, idxStr, '|');
            getline(ss2, hash, '|');
            getline(ss2, prevHash, '|');

            index = std::stoi(idxStr);
        }

        else if (type == "TX") {
            std::string id;
            getline(ss, id, '|');

            currentTx = Transaction();
            currentTx.id = id;

            txs.push_back(currentTx);
        }

        else if (type == "OUT") {
            std::string addr, amountStr;

            getline(ss, addr, '|');
            getline(ss, amountStr, '|');

            TxOutput out;
            out.address = addr;
            out.amount = std::stod(amountStr);

            txs.back().outputs.push_back(out);
        }
    }

    // 🔥 substitui chain inteira
    bc = Blockchain();

    for (const auto& b : loadedBlocks) {
        bc.addBlock(b);
    }

    bc.rebuildUTXO();

    file.close();
}
