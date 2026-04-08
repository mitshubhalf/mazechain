#include "../include/storage.h"
#include "../include/blockchain.h"
#include "../include/block.h"

#include <fstream>
#include <sstream>
#include <iostream>

namespace Storage {

// 💾 SAVE
void saveChain(const Blockchain &bc, const std::string& filename) {

    std::ofstream file(filename, std::ios::out | std::ios::trunc);

    if (!file.is_open()) {
        std::cout << "Erro ao salvar\n";
        return;
    }

    const std::vector<Block>& chain = bc.getChain();

    for (const Block& block : chain) {

        // 🔥 formato mais seguro (linha simples)
        file << block.index << "|"
             << block.timestamp << "|"
             << block.hash << "|"
             << block.previousHash << "|"
             << block.nonce
             << "\n";
    }

    file.flush();   // 🔥 garante escrita
    file.close();
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
    int loadedCount = 0;

    while (std::getline(file, line)) {

        if (line.empty()) continue;

        std::stringstream ss(line);

        std::string indexStr, timestamp, hash, prevHash, nonceStr;

        // 🔥 parse simples (sem falhar por besteira)
        std::getline(ss, indexStr, '|');
        std::getline(ss, timestamp, '|');
        std::getline(ss, hash, '|');
        std::getline(ss, prevHash, '|');
        std::getline(ss, nonceStr);

        try {
            int index = std::stoi(indexStr);
            int nonce = std::stoi(nonceStr);

            Block b(index, {}, prevHash);
            b.timestamp = timestamp;
            b.hash = hash;
            b.nonce = nonce;

            bc.addLoadedBlock(b);
            loadedCount++;

        } catch (...) {
            std::cout << "⚠️ Linha ignorada: " << line << "\n";
            continue;
        }
    }

    file.close();

    // 🔥 SE NÃO CARREGOU NADA → cria genesis
    if (loadedCount == 0) {
        std::cout << "Chain inválida, recriando genesis\n";
        bc.clearChain();
        bc.addLoadedBlock(Block(0, {}, "0"));
    }
}

} // namespace Storage
