#include "../include/storage.h"
#include <fstream>
#include <iostream>

static bool readInt(std::ifstream &f, int &out) {
    f >> out;
    if (f.fail()) return false;
    f.ignore(10000, '\n');
    return true;
}

void Storage::saveChain(const Blockchain& bc, const std::string& filename) {

    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cout << "Erro ao salvar blockchain\n";
        return;
    }

    const auto& chain = bc.getChain();

    for (const auto& block : chain) {

        file << "BLOCK\n";
        file << block.index << "\n";
        file << block.prevHash << "\n";
        file << block.hash << "\n";
        file << block.nonce << "\n";

        file << block.transactions.size() << "\n";

        for (const auto& tx : block.transactions) {

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

    Block* block = nullptr;

    while (std::getline(file, line)) {

        if (line != "BLOCK") continue;

        delete block;
        block = nullptr;

        int index, nonce;
        std::string prevHash, hash;

        if (!readInt(file, index)) break;
        std::getline(file, prevHash);
        std::getline(file, hash);
        if (!readInt(file, nonce)) break;

        block = new Block(index, prevHash, {});
        block->hash = hash;
        block->nonce = nonce;

        int txCount;
        if (!readInt(file, txCount)) break;

        if (txCount < 0 || txCount > 1000) continue;

        for (int i = 0; i < txCount; i++) {

            int vinSize;
            if (!readInt(file, vinSize)) break;

            std::vector<TxIn> vin;

            for (int j = 0; j < vinSize; j++) {
                TxIn in;
                std::getline(file, in.txid);
                file >> in.index;
                file.ignore(10000, '\n');
                vin.push_back(in);
            }

            int voutSize;
            if (!readInt(file, voutSize)) break;

            std::vector<TxOut> vout;

            for (int j = 0; j < voutSize; j++) {
                TxOut out;
                std::getline(file, out.address);
                file >> out.amount;
                file.ignore(10000, '\n');
                vout.push_back(out);
            }

            Transaction tx(vin, vout);
            block->transactions.push_back(tx);
        }

        // 🔥 validação final
        std::string target(bc.getDifficulty(), '0');

        if (block->hash != block->calculateHash()) {
            std::cout << "❌ bloco corrompido ignorado\n";
            continue;
        }

        if (block->hash.substr(0, bc.getDifficulty()) != target) {
            std::cout << "❌ PoW inválido ignorado\n";
            continue;
        }

        auto chain = bc.getChain();

        if (!chain.empty()) {
            if (block->prevHash != chain.back().hash) {
                std::cout << "❌ fork ignorado\n";
                continue;
            }
        }

        bc.addBlock(*block);
    }

    delete block;
}
