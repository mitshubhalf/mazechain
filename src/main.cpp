#include "../include/blockchain.h"
#include "../include/storage.h"
#include <iostream>

int main(int argc, char* argv[]) {
    Blockchain mazechain;

    // 🔥 Carrega do arquivo
    loadChain(mazechain);

    // 🔥 Se não existir, cria genesis
    if (mazechain.chain.empty()) {
        mazechain.chain.push_back(mazechain.createGenesisBlock());
    }

    if (argc < 2) {
        std::cout << "Uso:\n";
        std::cout << "./mazechain mine\n";
        std::cout << "./mazechain chain\n";
        return 0;
    }

    std::string cmd = argv[1];

    // ⛏️ MINERAR
    if (cmd == "mine") {
        Block newBlock(
            mazechain.chain.size(),
            {"Mining reward"},
            mazechain.getLatestBlock().hash
        );

        mazechain.addBlock(newBlock);

        saveChain(mazechain);
    }

    // 📜 VER CHAIN
    else if (cmd == "chain") {
        for (auto &block : mazechain.chain) {
            std::cout << "\n-------------------------\n";
            std::cout << "Index: " << block.index << "\n";
            std::cout << "Timestamp: " << block.timestamp << "\n";
            std::cout << "Hash: " << block.hash << "\n";
            std::cout << "Prev Hash: " << block.previousHash << "\n";
            std::cout << "Nonce: " << block.nonce << "\n";
        }
    }

    else {
        std::cout << "Comando desconhecido\n";
    }

    return 0;
}
