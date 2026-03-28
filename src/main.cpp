#include <iostream>
#include "../include/blockchain.h"
#include "../include/wallet.h"

int main(int argc, char* argv[]) {
    Blockchain blockchain;

    if (argc < 2) {
        std::cout << "Uso:\n";
        std::cout << "./mazechain wallet\n";
        std::cout << "./mazechain mine ENDERECO\n";
        std::cout << "./mazechain balance ENDERECO\n";
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "wallet") {
        Wallet w = createWallet();

        std::cout << "Address: " << w.address << "\n";
        std::cout << w.privateKey << "\n";
    }

    else if (cmd == "mine") {
        if (argc < 3) {
            std::cout << "Informe endereço\n";
            return 0;
        }

        std::string addr = argv[2];
        blockchain.mineBlock(addr);
    }

    else if (cmd == "balance") {
        if (argc < 3) {
            std::cout << "Informe endereço\n";
            return 0;
        }

        std::string addr = argv[2];
        std::cout << blockchain.getBalance(addr) << "\n";
    }

    return 0;
}
