#include <iostream>
#include "wallet.h"
#include "blockchain.h"

int main(int argc, char* argv[]) {

    if(argc < 2) {
        std::cout << "./mazechain createwallet\n";
        std::cout << "./mazechain mine\n";
        std::cout << "./mazechain balance\n";
        std::cout << "./mazechain validate\n";
        return 0;
    }

    std::string cmd = argv[1];

    if(cmd == "createwallet") {
        Wallet w;
        w.generateMnemonic();
        w.generateFromMnemonic();
        w.saveToFile("wallet.dat");

        std::cout << "Wallet criada\n";
        std::cout << "Seed: " << w.mnemonic << "\n";
        std::cout << "Address: " << w.address << "\n";
    }

    if(cmd == "mine") {
        Wallet w;
        w.loadFromFile("wallet.dat");

        Blockchain bc;
        bc.mineBlock(w.address);
    }

    if(cmd == "balance") {
        Wallet w;
        w.loadFromFile("wallet.dat");

        Blockchain bc;
        std::cout << "Balance: " << bc.getBalance(w.address) << " MC\n";
    }

    if(cmd == "validate") {
        Blockchain bc;
        std::cout << (bc.isValid() ? "Chain válida\n" : "Chain inválida\n");
    }

    return 0;
}
