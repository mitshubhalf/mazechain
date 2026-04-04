#include <iostream>
#include <string>
#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/transaction.h"

int main(int argc, char* argv[]) {

    Blockchain mazechain;

    // 🔹 Carrega blockchain do arquivo
    Storage::loadChain(mazechain);

    if (argc < 2) {
        std::cout << "Comandos disponíveis:\n";
        std::cout << "  mine\n";
        std::cout << "  chain\n";
        std::cout << "  send FROM TO AMOUNT\n";
        std::cout << "  balance ADDRESS\n";
        return 0;
    }

    std::string command = argv[1];

    // =========================
    // ⛏️ MINERAR
    // =========================
    if (command == "mine") {

        int index = mazechain.getChain().size();
        std::string prevHash = mazechain.getLatestBlock().hash;

        Block newBlock(index, {}, prevHash);

        std::cout << "⛏️ Mining block " << index << "...\n";
        newBlock.mineBlock(4);

        mazechain.addBlock(newBlock);

        // salva
        Storage::saveChain(mazechain);

        std::cout << "✅  Block mined: " << newBlock.hash << "\n";
    }

    // =========================
    // 📜 MOSTRAR CHAIN
    // =========================
    else if (command == "chain") {

        const auto& chain = mazechain.getChain();

        for (const auto& block : chain) {
            std::cout << "\n-------------------------\n";
            std::cout << "Index: " << block.index << "\n";
            std::cout << "Timestamp: " << block.timestamp << "\n";
            std::cout << "Hash: " << block.hash << "\n";
            std::cout << "Prev Hash: " << block.previousHash << "\n";
            std::cout << "Nonce: " << block.nonce << "\n";

            // mostrar transações
            for (const auto& tx : block.transactions) {
                std::cout << "   TX: " << tx.toString() << "\n";
            }
        }
    }

    // =========================
    // 💸 ENVIAR
    // =========================
    else if (command == "send") {

        if (argc < 5) {
            std::cout << "Uso: send FROM TO AMOUNT\n";
            return 0;
        }

        std::string from = argv[2];
        std::string to = argv[3];
        double amount = std::stod(argv[4]);

        Transaction tx(from, to, amount);

        mazechain.addTransaction(tx);

        // salva mesmo sem minerar (fica no último bloco)
        Storage::saveChain(mazechain);

        std::cout << "✅ Transação enviada\n";
    }

    // =========================
    // 💰 SALDO
    // =========================
    else if (command == "balance") {

        if (argc < 3) {
            std::cout << "Uso: balance ADDRESS\n";
            return 0;
        }

        std::string address = argv[2];
        double balance = mazechain.getBalance(address);

        std::cout << "💰 Saldo: " << balance << "\n";
    }

    // =========================
    // ❌ ERRO
    // =========================
    else {
        std::cout << "Comando desconhecido\n";
    }

    return 0;
}
