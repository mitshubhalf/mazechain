#include "../include/storage.h"
#include <fstream>
#include <iostream>

static bool safeReadInt(std::ifstream &file, int &out) {
    if (!(file >> out)) return false;
    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return true;
}

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

    if (!file.is_open()) return;

    bc.clearChain();

    std::string line;
    Block* currentBlock = nullptr;

    while (std::getline(file, line)) {

        if (line == "BLOCK") {

            delete currentBlock;
            currentBlock = nullptr;

            int index, nonce;
            std::string prevHash, hash;

            if (!safeReadInt(file, index)) break;
            std::getline(file, prevHash);
            std::getline(file, hash);
            if (!safeReadInt(file, nonce)) break;

            currentBlock = new Block(index, prevHash, {});
            currentBlock->hash = hash;
            currentBlock->nonce = nonce;
        }

        else if (line == "TX") {
            if (!currentBlock) continue;

            int vinSize;
            if (!safeReadInt(file, vinSize)) break;
            if (vinSize < 0 || vinSize > 500) continue;

            std::vector<TxIn> vin;

            for (int i = 0; i < vinSize; i++) {
                TxIn in;
                std::getline(file, in.txid);

                int idx;
                if (!safeReadInt(file, idx)) break;
                in.index = idx;

                vin.push_back(in);
            }

            int voutSize;
            if (!safeReadInt(file, voutSize)) break;
            if (voutSize < 0 || voutSize > 500) continue;

            std::vector<TxOut> vout;

            for (int i = 0; i < voutSize; i++) {
                TxOut out;
                std::getline(file, out.address);

                double amount;
                file >> amount;
                file.ignore();
                out.amount = amount;

                vout.push_back(out);
            }

            Transaction tx(vin, vout);
            currentBlock->transactions.push_back(tx);
        }

        else if (line == "END_BLOCK") {

            if (!currentBlock) continue;

            // 🔥 validação mínima segura

            if (currentBlock->hash != currentBlock->calculateHash()) {
                std::cout << "❌ Bloco corrompido ignorado\n";
                delete currentBlock;
                currentBlock = nullptr;
                continue;
            }

            std::string target(bc.getDifficulty(), '0');

            if (currentBlock->hash.substr(0, bc.getDifficulty()) != target) {
                std::cout << "❌ PoW inválido ignorado\n";
                delete currentBlock;
                currentBlock = nullptr;
                continue;
            }

            auto chain = bc.getChain();

            if (!chain.empty()) {
                if (currentBlock->prevHash != chain.back().hash) {
                    std::cout << "❌ Fork ignorado\n";
                    delete currentBlock;
                    currentBlock = nullptr;
                    continue;
                }
            }

            bc.addBlock(*currentBlock);

            delete currentBlock;
            currentBlock = nullptr;
        }
    }

    delete currentBlock;
}
