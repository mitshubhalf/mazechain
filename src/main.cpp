#include <iostream>
#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/transaction.h"

int main(int argc, char* argv[]) {

    Blockchain mazechain;

    // carregar blockchain salva
    Storage::loadChain(mazechain);

    if (argc < 2) {
        std::cout << "Uso:\n";
        std::cout << "./mazechain mine\n";
        std::cout << "./mazechain chain\n";
        std::cout << "./mazechain balance <address>\n";
        std::cout << "./mazechain send <from> <to> <amount>\n";
        return 0;
    }

    std::string command = argv[1];

    // ========================
    // MINERAR BLOCO
    // ========================
    if (command == "mine") {

        std::cout << "⛏️ Mining block " 
                  << mazechain.getChain().size() << "...\n";

        mazechain.minePendingTransactions("miner1");

        Storage::saveChain(mazechain);

        std::cout << "✅ Block mined!\n";
    }

    // ========================
    // MOSTRAR CHAIN
    // ========================
    else if (command == "chain") {

        const auto& chain = mazechain.getChain();

        for (const auto& block : chain) {
            std::cout << "\n-------------------------\n";
            std::cout << "Index: " << block.index << "\n";
            std::cout << "Timestamp: " << block.timestamp << "\n";
            std::cout << "Hash: " << block.hash << "\n";
            std::cout << "Prev Hash: " << block.previousHash << "\n";
            std::cout << "Nonce: " << block.nonce << "\n";
        }
    }

    // ========================
    // VER SALDO
    // ========================
    else if (command == "balance") {

        if (argc < 3) {
            std::cout << "Uso: ./mazechain balance <address>\n";
            return 0;
        }

        std::string address = argv[2];

        int balance = mazechain.getBalance(address);

        std::cout << "💰 Saldo de " << address << ": "
                  << balance << " MC\n";
    }

    // ========================
    // ENVIAR TRANSAÇÃO
    // ========================
    else if (command == "send") {

        if (argc < 5) {
            std::cout << "Uso: ./mazechain send <from> <to> <amount>\n";
            return 0;
        }

        std::string from = argv[2];
        std::string to = argv[3];
        int amount = std::stoi(argv[4]);

        Transaction tx(from, to, amount);

        mazechain.addTransaction(tx);

        std::cout << "📤 Transação adicionada!\n";
    }

    else {
        std::cout << "Comando desconhecido\n";
    }

    return 0;
}
