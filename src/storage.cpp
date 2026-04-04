#include "../include/storage.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace Storage {

void loadChain(Blockchain &bc, const std::string& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) return;

    bc.clearChain();

    std::string line;

    while (std::getline(file, line)) {

        if (line.empty()) continue;

        std::stringstream ss(line);

        std::string indexStr, timestamp, hash, prevHash, nonceStr;

        // lê até '|'
        std::getline(ss, indexStr, '|');
        std::getline(ss, timestamp, '|');
        std::getline(ss, hash, '|');
        std::getline(ss, prevHash, '|');

        // ⚠️ último campo SEM delimitador
        std::getline(ss, nonceStr);

        try {
            int index = std::stoi(indexStr);
            int nonce = std::stoi(nonceStr);

            Block b(index, {}, prevHash);
            b.timestamp = timestamp;
            b.hash = hash;
            b.nonce = nonce;

            bc.addLoadedBlock(b);

        } catch (...) {
            std::cout << "⚠️ Linha inválida ignorada: " << line << "\n";
        }
    }
}

}
