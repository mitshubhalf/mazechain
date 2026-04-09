#include "../include/storage.h"
#include <fstream>
#include <iostream>

void Storage::saveChain(const Blockchain& bc, const std::string& filename) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cout << "Erro ao salvar blockchain\n";
        return;
    }

    auto chain = bc.getChain();

    for (const auto& block : chain) {
        file << "BLOCK\n";
        file << block.index << "\n";
        file << block.prevHash << "\n";
        file << block.hash << "\n";
        file << block.nonce << "\n";

        for (const auto& tx : block.transactions) {
            file << "TX\n";

            file << tx.vin.size() << "\n";
            for (const auto& in : tx.vin) {
                file << in.txid << "\n";
                file << in.index << "\n";
            }

            file << tx.vout.size() << "\n";
            for (const auto& out : tx.vout) {
                file << out.address << "\n";
                file << out.amount << "\n";
            }
        }

        file << "END_BLOCK\n";
    }
}

void Storage::loadChain(Blockchain& bc, const std::string& filename) {
    std::ifstream file(filename);

    if (!file.is_open())
        return;

    bc.clearChain();

    std::string line;
    Block* current = nullptr;

    while (std::getline(file, line)) {

        if (line == "BLOCK") {
            int index, nonce;
            std::string prevHash, hash;

            file >> index;
            file.ignore();

            std::getline(file, prevHash);
            std::getline(file, hash);

            file >> nonce;
            file.ignore();

            current = new Block(index, prevHash, {});
            current->nonce = nonce;
            current->hash = hash;
        }

        else if (line == "TX") {
            int vinSize, voutSize;

            file >> vinSize;
            file.ignore();

            std::vector<TxIn> vin;
            for (int i = 0; i < vinSize; i++) {
                TxIn in;
                std::getline(file, in.txid);
                file >> in.index;
                file.ignore();
                vin.push_back(in);
            }

            file >> voutSize;
            file.ignore();

            std::vector<TxOut> vout;
            for (int i = 0; i < voutSize; i++) {
                TxOut out;
                std::getline(file, out.address);
                file >> out.amount;
                file.ignore();
                vout.push_back(out);
            }

            current->transactions.emplace_back(vin, vout);
        }

        else if (line == "END_BLOCK") {

            // 🔥 VALIDAÇÃO FINAL REAL

            if (current->hash != current->calculateHash()) {
                std::cout << "❌ Bloco corrompido ignorado\n";
                delete current;
                continue;
            }

            std::string target(bc.getDifficulty(), '0');

            if (current->hash.substr(0, bc.getDifficulty()) != target) {
                std::cout << "❌ PoW inválido no storage\n";
                delete current;
                continue;
            }

            auto chain = bc.getChain();

            if (!chain.empty() &&
                current->prevHash != chain.back().hash) {
                std::cout << "❌ Fork inválido no storage\n";
                delete current;
                continue;
            }

            bc.addBlock(*current);
            delete current;
        }
    }
}
