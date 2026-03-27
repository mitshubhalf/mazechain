#include <iostream>
#include "../include/blockchain.h"
#include "../include/wallet.h"

int main(int argc, char* argv[]) {

    Blockchain blockchain;

    if (argc < 2) {
        std::cout << "./mazechain wallet\n";
        std::cout << "./mazechain mine ADDRESS\n";
        std::cout << "./mazechain balance ADDRESS\n";
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "wallet") {

        Wallet w = createWallet();

        std::cout << "Address: " << w.address << "\n";
        std::cout << "Private: " << w.privateKey << "\n";
    }

    else if (cmd == "mine") {

        blockchain.mineBlock(argv[2]);
    }

    else if (cmd == "balance") {

        std::cout << blockchain.getBalance(argv[2]) << "\n";
    }

    return 0;
}
