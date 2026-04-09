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
        file << block.prevHash << "\n";
        file << block.hash << "\n";
        file << block.nonce << "\n";

        for (const auto& tx : block.transactions) {
            file << "TX\n";

            // INPUTS
            file << tx.vin.size() << "\n";
            for (const auto& in : tx.vin) {
                file << in.txid << "\n";
                file << in.vout << "\n";
            }

            // OUTPUTS
            file << tx.vout.size() << "\n";
            for (const auto& out : tx.vout) {
                file << out.address << "\n";
                file << out.amount << "\n";
            }
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
            std::vector<TxIn> vin;
            std::vector<TxOut> vout;

            int vinSize;
            file >> vinSize;
            file.ignore();

            for (int i = 0; i < vinSize; i++) {
                TxIn in;
                std::getline(file, in.txid);
                file >> in.vout;
                file.ignore();
                vin.push_back(in);
            }

            int voutSize;
            file >> voutSize;
            file.ignore();

            for (int i = 0; i < voutSize; i++) {
                TxOut out;
                std::getline(file, out.address);
                file >> out.amount;
                file.ignore();
                vout.push_back(out);
            }

            Transaction tx(vin, vout);
            currentBlock->transactions.push_back(tx);
        }

        else if (line == "END_BLOCK") {
            bc.chain.push_back(*currentBlock);
            delete currentBlock;
        }
    }

    file.close();
}
