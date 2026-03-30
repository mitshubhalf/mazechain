#include "../include/blockchain.h"
#include <iostream>

int main() {
    Blockchain mazechain;

    mazechain.addBlock(Block(1, {"A -> B: 10 MC"}, mazechain.getLatestBlock().hash));
    mazechain.addBlock(Block(2, {"B -> C: 5 MC"}, mazechain.getLatestBlock().hash));

    std::cout << "\n=== MAZECHAIN ===\n";

    for (auto &block : mazechain.chain) {
        std::cout << "\n-------------------------\n";
        std::cout << "Index: " << block.index << "\n";
        std::cout << "Timestamp: " << block.timestamp;
        std::cout << "Hash: " << block.hash << "\n";
        std::cout << "Prev Hash: " << block.previousHash << "\n";
        std::cout << "Nonce: " << block.nonce << "\n";
    }

    return 0;
}
