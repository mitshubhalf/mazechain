#include "../include/blockchain.h"
#include "../include/storage.h"
#include <iostream>

int main(int argc, char* argv[]) {
    Blockchain mazechain;

    // 🔥 carregar do arquivo
    Storage::loadChain(mazechain);

    // 🔥 criar genesis se vazio
    if (mazechain.getChain().empty()) {
        mazechain.addLoadedBlock(mazechain.createGenesisBlock());
    }

    if (argc < 2) {
        std::cout << "Uso:\n";
        std::cout << "./mazechain mine\n";
        std::cout << "./mazechain chain\n";
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "mine") {
        Block newBlock(
            mazechain.getChain().size(),
            {"Mining reward"},
            mazechain.getLatestBlock().hash
        );

        mazechain.addBlock(newBlock);

        Storage::saveChain(mazechain);
    }

    else if (cmd == "chain") {
        const auto& chain = mazechain.getChain();

        for (auto &block : chain) {
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
