#include "../include/blockchain.h"
#include "../include/storage.h"
#include <iostream>

int main(int argc, char* argv[]) {

    Blockchain mazechain;
    Storage::loadChain(mazechain);

    if (argc < 2) {
        std::cout << "Comandos:\n";
        std::cout << " mine\n";
        std::cout << " send FROM TO AMOUNT\n";
        std::cout << " balance ADDRESS\n";
        std::cout << " chain\n";
        return 0;
    }

    std::string command = argv[1];

    // ⛏️ MINERAR
    if (command == "mine") {
        mazechain.minePendingTransactions("miner1");
        Storage::saveChain(mazechain);
    }

    // 💸 ENVIAR
    else if (command == "send") {

        if (argc < 5) {
            std::cout << "Uso: send FROM TO AMOUNT\n";
            return 0;
        }

        Transaction tx;
        tx.from = argv[2];
        tx.to = argv[3];
        tx.amount = std::stoi(argv[4]);

        if (mazechain.addTransaction(tx))
            std::cout << "✅ Transação adicionada\n";
    }

    // 💰 SALDO
    else if (command == "balance") {

        if (argc < 3) {
            std::cout << "Uso: balance ADDRESS\n";
            return 0;
        }

        std::string addr = argv[2];

        std::cout << "Saldo: "
                  << mazechain.getBalance(addr)
                  << " MC\n";
    }

    // 🔗 VER CHAIN
    else if (command == "chain") {
        mazechain.printChain();
    }

    else {
        std::cout << "Comando desconhecido\n";
    }

    return 0;
}
