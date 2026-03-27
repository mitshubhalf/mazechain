#include <iostream>
#include "../include/blockchain.h"
#include "../include/wallet.h"
#include "../include/crypto.h"

int main(int argc, char* argv[]) {
    Blockchain blockchain;

    if (argc < 2) {
        std::cout << "Comandos:\n";
        std::cout << "wallet\n";
        std::cout << "mine <address>\n";
        std::cout << "balance <address>\n";
        std::cout << "send <from> <to> <amount> <privateKey>\n";
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "wallet") {
        Wallet w = createWallet();
        std::cout << "Address: " << w.address << "\n";
        std::cout << w.privateKey << "\n";
    }

    else if (cmd == "mine") {
        blockchain.mineBlock(argv[2]);
    }

    else if (cmd == "balance") {
        std::cout << blockchain.getBalance(argv[2]) << "\n";
    }

    else if (cmd == "send") {
        std::string from = argv[2];
        std::string to = argv[3];
        double amount = std::stod(argv[4]);
        std::string priv = argv[5];

        Transaction tx;
        tx.from = from;
        tx.to = to;
        tx.amount = amount;

        tx.signature = signData(tx.toString(), priv);

        blockchain.addTransaction(tx);
    }

    return 0;
}
