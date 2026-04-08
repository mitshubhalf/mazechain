#include "../include/storage.h"
#include "../include/blockchain.h"
#include "../include/block.h"

#include <fstream>
#include <sstream>
#include <iostream>

namespace Storage {

// 💾 SAVE
void saveChain(const Blockchain &bc, const std::string& filename) {

    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cout << "Erro ao salvar\n";
        return;
    }

    const std::vector<Block>& chain = bc.getChain();

    for (const Block& block : chain) {
        file << block.index << "|"
             << block.timestamp << "|"
             << block.hash << "|"
             << block.previousHash << "|"
             << block.nonce
             << "\n";
    }

    file.close(); // 🔥 GARANTE SALVAMENTO
}

// 📂 LOAD
void loadChain(Blockchain &bc, const std::string& filename) {

    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cout << "Nenhum arquivo encontrado, iniciando nova chain\n";
        return;
    }

    bc.clearChain();

    std::string line;
    bool loaded = false;

    while (std::getline(file, line)) {

        if (line.empty()) continue;

        std::stringstream ss(line);

        std::string indexStr, timestamp, hash, prevHash, nonceStr;

        if (!std::getline(ss, indexStr, '|')) continue;
        if (!std::getline(ss, timestamp, '|')) continue;
        if (!std::getline(ss, hash, '|')) continue;
        if (!std::getline(ss, prevHash, '|')) continue;
        if (!std::getline(ss, nonceStr)) continue;

        try {
            int index = std::stoi(indexStr);
            int nonce = std::stoi(nonceStr);

            if (index < 0 || nonce < 0) continue;
            if (hash.empty() || prevHash.empty()) continue;

            Block b(index, {}, prevHash);
            b.timestamp = timestamp;
            b.hash = hash;
            b.nonce = nonce;

            bc.addLoadedBlock(b);
            loaded = true;

        } catch (...) {
            continue;
        }
    }

    file.close();

    // 🔥 GARANTE QUE SEMPRE TEM PELO MENOS GENESIS
    if (!loaded || bc.getChain().empty()) {
        std::cout << "Chain inválida, recriando genesis\n";
        bc.clearChain();
        bc.addLoadedBlock(Block(0, {}, "0"));
    }
}

} // namespace Storage
