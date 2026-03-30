#include "../include/blockchain.h"
#include <iostream>

int main(int argc, char* argv[]) {
    Blockchain mazechain;

    if (argc < 2) {
        std::cout << "Uso:\n";
        std::cout << "./mazechain mine\n";
        std::cout << "./mazechain chain\n";
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "mine") {
        mazechain.addBlock(Block(
            mazechain.chain.size(),
            {"Mining reward"},
            mazechain.getLatestBlock().hash
        ));
    }

    else if (cmd == "chain") {
        for (auto &block : mazechain.chain) {
            std::cout << "\n-------------------------\n";
            std::cout << "Index: " << block.index << "\n";
            std::cout << "Hash: " << block.hash << "\n";
            std::cout << "Prev: " << block.previousHash << "\n";
            std::cout << "Nonce: " << block.nonce << "\n";
        }
    }

    else {
        std::cout << "Comando desconhecido\n";
    }

    return 0;
}
