#include <iostream>
#include "../include/blockchain.h"
#include "../include/wallet.h"

int main(int argc, char* argv[]) {
    Blockchain blockchain;

    if (argc < 2) {
        std::cout << "Uso:\n";
        std::cout << "./mazechain wallet\n";
        std::cout << "./mazechain mine <address>\n";
        std::cout << "./mazechain balance <address>\n";
        return 0;
    }

    std::string command = argv[1];

    // 🔥 WALLET
    if (command == "wallet") {
        Wallet w = createWallet();

        std::cout << "Address: " << w.address << "\n";
        std::cout << w.privateKey << "\n";

        return 0;
    }

    // 🔥 MINERAR
    if (command == "mine" && argc >= 3) {
        blockchain.mineBlock(argv[2]);
        return 0;
    }

    // 🔥 SALDO
    if (command == "balance" && argc >= 3) {
        std::cout << blockchain.getBalance(argv[2]) << "\n";
        return 0;
    }

    std::cout << "Comando inválido\n";
    return 0;
}
