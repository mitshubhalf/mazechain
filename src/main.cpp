#include "../include/blockchain.h"
#include <iostream>

int main(int argc, char* argv[]) {
    Blockchain bc;

    if (argc < 2) {
        std::cout << "Comandos: mine | balance | send\n";
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "mine") {
        bc.mineBlock(argv[2]);
    }
    else if (cmd == "balance") {
        std::cout << "💰 " << bc.getBalance(argv[2]) << "\n";
    }
    else if (cmd == "send") {
        bc.send(argv[2], argv[3], std::stod(argv[4]));
    }

    return 0;
}
