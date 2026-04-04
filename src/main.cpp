#include "../include/blockchain.h"
#include "../include/storage.h"
#include <iostream>

int main(int argc, char* argv[]) {

    Blockchain mazechain;
    Storage::loadChain(mazechain);

    if (argc < 2) {
        std::cout << "Comandos:\n";
        std::cout << " mine\n send FROM TO AMOUNT\n balance ADDRESS\n chain\n";
        return 0;
    }

    std::string command = argv[1];

    if (command == "mine") {
        mazechain.minePendingTransactions("miner1");
        Storage::saveChain(mazechain);
    }

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

        Storage::saveChain(mazechain);
    }

    else if (command == "balance") {

        std::string addr = argv[2];

        std::cout << "Saldo: "
                  << mazechain.getBalance(addr)
                  << " MC\n";
    }

    else if (command == "chain") {
        mazechain.printChain();
    }

    return 0;
}
