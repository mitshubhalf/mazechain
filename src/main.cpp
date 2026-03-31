#include "../include/blockchain.h"
#include "../include/storage.h"
#include <iostream>

int main(int argc, char* argv[]) {
    Blockchain mazechain;

    // 🔥 Carregar blockchain do arquivo
    loadChain(mazechain);

    if (argc < 2) {
        std::cout << "Uso:\n";
        std::cout << "./mazechain mine\n";
        std::cout << "./mazechain chain\n";
        return 0;
    }

    std::string cmd = argv[1];

    // ⛏️ MINERAR BLOCO
    if (cmd == "mine") {
        if (mazechain.isEmpty()) {
            std::cerr << "Blockchain vazia!\n";
            return 1;
        }

        Block newBlock(
            mazechain.getChain().size(),
            {"Mining reward"},
            mazechain.getLatestBlock().hash
        );

        mazechain.addBlock(newBlock);

        if (!mazechain.isChainValid()) {
            std::cerr << "Erro: blockchain inválida após mineração!\n";
            return 1;
        }

        // 💾 SALVAR
        saveChain(mazechain);
    }

    // 📜 MOSTRAR BLOCKCHAIN
    else if (cmd == "chain") {
        const auto& chain = mazechain.getChain();

        for (const auto &block : chain) {
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
