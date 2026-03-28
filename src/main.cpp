#include "../include/blockchain.h"
#include "../include/wallet.h"
#include <iostream>

int main(int argc, char* argv[]) {

    Blockchain blockchain;

    if (argc < 2) {
        std::cout << "Uso:\n";
        std::cout << "./mazechain wallet\n";
        std::cout << "./mazechain mine <address>\n";
        std::cout << "./mazechain balance <address>\n";
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "wallet") {
        Wallet w = createWallet();

        std::cout << "Address: " << w.address << "\n";
        std::cout << "Private Key: " << w.privateKey << "\n";
    }
    else if (cmd == "mine") {
        if (argc < 3) {
            std::cout << "Informe o endereço\n";
            return 0;
        }
        blockchain.mineBlock(argv[2]);
    }
    else if (cmd == "balance") {
        if (argc < 3) {
            std::cout << "Informe o endereço\n";
            return 0;
        }
        std::cout << blockchain.getBalance(argv[2]) << "\n";
    }

    return 0;
}
