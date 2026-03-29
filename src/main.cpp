#include "../include/blockchain.h"
#include <iostream>

int main() {
    Blockchain mazechain;

    std::cout << "⛏️ Minerando bloco 1...\n";
    mazechain.addBlock(Block(1, {"Tx1: A -> B (10 MC)"}, mazechain.getLatestBlock().hash));

    std::cout << "⛏️ Minerando bloco 2...\n";
    mazechain.addBlock(Block(2, {"Tx2: B -> C (5 MC)"}, mazechain.getLatestBlock().hash));

    for (auto &block : mazechain.chain) {
        std::cout << "\n====================\n";
        std::cout << "Index: " << block.index << "\n";
        std::cout << "Hash: " << block.hash << "\n";
        std::cout << "Prev: " << block.previousHash << "\n";
    }

    return 0;
}
