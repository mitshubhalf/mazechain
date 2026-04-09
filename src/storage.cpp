#include "../include/storage.h"
#include <fstream>
#include <iostream>
#include <vector>

void Storage::saveChain(const Blockchain& bc, const std::string& filename) {
    std::ofstream file(filename, std::ios::trunc);
    if (!file.is_open()) return;

    for (const auto& block : bc.getChain()) {
        file << "BLOCK\n" 
             << block.index << "\n" 
             << block.prevHash << "\n" 
             << block.hash << "\n" 
             << block.nonce << "\n"
             << block.timestamp << "\n"; // 🔥 SALVANDO O TIMESTAMP

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

    while (std::getline(file >> std::ws, line)) {
        if (line == "BLOCK") {
            int tIdx, tNonce;
            long tTime;
            std::string tPrev, tHash;
            std::vector<Transaction> tTxs;

            if (!(file >> tIdx)) break;
            file.ignore(1000, '\n');
            std::getline(file >> std::ws, tPrev);
            std::getline(file >> std::ws, tHash);
            if (!(file >> tNonce)) break;
            if (!(file >> tTime)) break; // 🔥 LENDO O TIMESTAMP
            file.ignore(1000, '\n');

            std::string subLine;
            while (file >> subLine && subLine != "END_BLOCK") {
                if (subLine == "TX") {
                    int vinS, voutS;
                    file >> vinS; file.ignore(1000, '\n');
                    std::vector<TxIn> vins;
                    for (int i = 0; i < vinS; i++) {
                        TxIn in;
                        std::getline(file >> std::ws, in.txid);
                        file >> in.index; file.ignore(1000, '\n');
                        vins.push_back(in);
                    }
                    file >> voutS; file.ignore(1000, '\n');
                    std::vector<TxOut> vouts;
                    for (int i = 0; i < voutS; i++) {
                        TxOut out;
                        std::getline(file >> std::ws, out.address);
                        file >> out.amount; file.ignore(1000, '\n');
                        vouts.push_back(out);
                    }
                    tTxs.push_back(Transaction(vins, vouts));
                }
            }

            Block loadedBlock(tIdx, tPrev, tTxs);
            loadedBlock.hash = tHash;
            loadedBlock.nonce = tNonce;
            loadedBlock.timestamp = tTime; // 🔥 REAPLICANDO O TIMESTAMP ORIGINAL

            if (loadedBlock.hash == loadedBlock.calculateHash()) {
                bc.addBlock(loadedBlock);
            } else {
                std::cerr << "❌ Bloco " << tIdx << " corrompido (Hash mismatch).\n";
            }
        }
    }
    file.close();
}
