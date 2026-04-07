#include <iostream>
#include <string>
#include "blockchain.h"
#include "include/transaction.h"

int main(int argc, char* argv[]) {

    Blockchain mazechain;

    if (argc < 2) {
        std::cout << "Comandos disponíveis:\n";
        std::cout << "  mine\n";
        std::cout << "  chain\n";
        std::cout << "  send FROM TO AMOUNT\n";
        std::cout << "  balance ADDRESS\n";
        return 0;
    }

    std::string command = argv[1];

    if (command == "mine") {
        std::cout << "⛏️ Mining...\n";
        mazechain.minePendingTransactions("miner1");
        std::cout << "✅ Block mined!\n";
    }

    else if (command == "chain") {
        const auto& chain = mazechain.getChain();

        for (const auto& block : chain) {
            std::cout << "\nIndex: " << block.index << "\n";
            std::cout << "Hash: " << block.hash << "\n";
            std::cout << "Prev: " << block.previousHash << "\n";
        }
    }

    else if (command == "send") {

        if (argc < 5) {
            std::cout << "Uso: send FROM TO AMOUNT\n";
            return 0;
        }

        std::string from = argv[2];
        std::string to = argv[3];
        double amount = std::stod(argv[4]);

        mazechain.addTransaction(Transaction(from, to, amount));

        std::cout << "✅ Transação adicionada\n";
    }

    else if (command == "balance") {

        std::string addr = argv[2];
        std::cout << "Saldo: " << mazechain.getBalance(addr) << "\n";
    }

    return 0;
}
