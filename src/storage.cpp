#include "../include/storage.h"
#include <fstream>
#include <iostream>

void Storage::saveChain(const Blockchain& bc, const std::string& filename) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cout << "Erro ao salvar blockchain\n";
        return;
    }

    std::vector<Block> chain = bc.getChain();

    for (const auto& block : chain) {
        file << "BLOCK\n";
        file << block.index << "\n";
        file << block.prevHash << "\n";
        file << block.hash << "\n";
        file << block.nonce << "\n";
        file << block.timestamp << "\n"; // 🔥 CORREÇÃO CRÍTICA

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

    file.close();
}

void Storage::loadChain(Blockchain& bc, const std::string& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) return;

    bc.clearChain();

    std::string line;
    Block* currentBlock = nullptr;

    while (std::getline(file, line)) {

        if (line == "BLOCK") {
            int index;
            std::string prevHash, hash;
            int nonce;
            long timestamp;

            file >> index;
            file.ignore();

            std::getline(file, prevHash);
            std::getline(file, hash);

            file >> nonce;
            file.ignore();

            file >> timestamp;       // 🔥 NOVO
            file.ignore();

            currentBlock = new Block(index, prevHash, {});
            currentBlock->hash = hash;
            currentBlock->nonce = nonce;
            currentBlock->timestamp = timestamp; // 🔥 CRÍTICO
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
                file >> in.index;
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

            // 🔥 valida hash
            if (currentBlock->hash != currentBlock->calculateHash()) {
                std::cout << "❌ Hash inválido detectado!\n";
                delete currentBlock;
                continue;
            }

            // 🔥 valida PoW
            std::string target(bc.getDifficulty(), '0');
            if (currentBlock->hash.substr(0, bc.getDifficulty()) != target) {
                std::cout << "❌ PoW inválido!\n";
                delete currentBlock;
                continue;
            }

            // 🔥 valida encadeamento
            std::vector<Block> currentChain = bc.getChain();
            if (!currentChain.empty()) {
                if (currentBlock->prevHash != currentChain.back().hash) {
                    std::cout << "❌ Blockchain quebrada!\n";
                    delete currentBlock;
                    continue;
                }
            }

            bc.addBlock(*currentBlock);
            delete currentBlock;
        }
    }

    file.close();
}
