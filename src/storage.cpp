#include "../include/storage.h"
#include <fstream>
#include <iostream>

void Storage::saveChain(const Blockchain& bc, const std::string& filename) {
    std::ofstream file(filename, std::ios::trunc);
    if (!file.is_open()) return;

    file << "DIFFICULTY " << bc.getDifficulty() << "\n";

    for (const auto& block : bc.getChain()) {
        file << "BLOCK\n" << block.index << "\n" << block.prevHash << "\n" 
             << block.hash << "\n" << block.nonce << "\n" << block.timestamp << "\n";

        for (const auto& tx : block.transactions) {
            file << "TX\n" << tx.vin.size() << "\n";
            for (const auto& in : tx.vin) file << in.txid << "\n" << in.index << "\n";
            file << tx.vout.size() << "\n";
            for (const auto& out : tx.vout) file << out.address << "\n" << out.amount << "\n";
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

    while (file >> line) {
        if (line == "DIFFICULTY") {
            int d; file >> d; bc.setDifficulty(d);
        } else if (line == "BLOCK") {
            int tIdx, tNonce; long tTime; std::string tPrev, tHash;
            file >> tIdx; file.ignore(1000, '\n');
            std::getline(file >> std::ws, tPrev);
            std::getline(file >> std::ws, tHash);
            file >> tNonce >> tTime;

            std::vector<Transaction> tTxs;
            std::string subLine;
            while (file >> subLine && subLine != "END_BLOCK") {
                if (subLine == "TX") {
                    int vinS, voutS; file >> vinS;
                    std::vector<TxIn> vins;
                    for (int i = 0; i < vinS; i++) {
                        TxIn in; file >> std::ws; std::getline(file, in.txid);
                        file >> in.index; vins.push_back(in);
                    }
                    file >> voutS;
                    std::vector<TxOut> vouts;
                    for (int i = 0; i < voutS; i++) {
                        TxOut out; file >> std::ws; std::getline(file, out.address);
                        file >> out.amount; vouts.push_back(out);
                    }
                    tTxs.push_back(Transaction(vins, vouts));
                }
            }
            Block loadedBlock(tIdx, tPrev, tTxs);
            loadedBlock.hash = tHash; loadedBlock.nonce = tNonce; loadedBlock.timestamp = tTime;
            bc.addBlock(loadedBlock);
        }
    }
}
