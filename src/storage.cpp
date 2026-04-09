#include "../include/storage.h"
#include <fstream>
#include <iostream>
#include <vector>

void Storage::saveChain(const Blockchain& bc, const std::string& filename) {
    std::ofstream file(filename, std::ios::trunc);
    if (!file.is_open()) return;

    for (const auto& block : bc.getChain()) {
        file << "BLOCK\n" << block.index << "\n" << block.prevHash << "\n" 
             << block.hash << "\n" << block.nonce << "\n";

        for (const auto& tx : block.transactions) {
            file << "TX\n" << tx.vin.size() << "\n";
            for (const auto& in : tx.vin) {
                file << in.txid << "\n" << in.index << "\n";
            }
            file << tx.vout.size() << "\n";
            for (const auto& out : tx.vout) {
                file << out.address << "\n" << out.amount << "\n";
            }
        }
        file << "END_BLOCK\n";
    }
    file.close();
}

void Storage::loadChain(Blockchain& bc, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    bc.clearChain();
    std::string line;

    int tIdx, tNonce;
    std::string tPrev, tHash;
    std::vector<Transaction> tTxs;

    while (std::getline(file >> std::ws, line)) {
        if (line == "BLOCK") {
            tTxs.clear();
            if (!(file >> tIdx)) break;
            file.ignore(1000, '\n');
            std::getline(file, tPrev);
            std::getline(file, tHash);
            if (!(file >> tNonce)) break;
            file.ignore(1000, '\n');
        } 
        else if (line == "TX") {
            int vinS, voutS;
            if (!(file >> vinS)) break;
            file.ignore(1000, '\n');

            if (vinS < 0 || vinS > 100) continue; // Proteção extra

            std::vector<TxIn> vins;
            for (int i = 0; i < vinS; i++) {
                TxIn in;
                std::getline(file, in.txid);
                file >> in.index; file.ignore(1000, '\n');
                vins.push_back(in);
            }

            if (!(file >> voutS)) break;
            file.ignore(1000, '\n');
            if (voutS < 0 || voutS > 100) continue;

            std::vector<TxOut> vouts;
            for (int i = 0; i < voutS; i++) {
                TxOut out;
                std::getline(file, out.address);
                if (!(file >> out.amount)) break;
                file.ignore(1000, '\n');
                vouts.push_back(out);
            }
            tTxs.push_back(Transaction(vins, vouts));
        } 
        else if (line == "END_BLOCK") {
            Block newBlock(tIdx, tPrev, tTxs);
            newBlock.hash = tHash;
            newBlock.nonce = tNonce;

            if (newBlock.hash == newBlock.calculateHash()) {
                bc.addBlock(newBlock);
            } else {
                std::cerr << "❌ Bloco " << tIdx << " corrompido e ignorado.\n";
            }
        }
    }
    file.close();
}
