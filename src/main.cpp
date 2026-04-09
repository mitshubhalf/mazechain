#include <iostream>
#include <cstdlib>
#include "blockchain.h"
#include "storage.h"

int main(int argc, char* argv[]) {

    Blockchain mazechain;

    Storage::loadChain(mazechain, "chain.dat");

    // 🔥 ESSENCIAL
    mazechain.rebuildUTXO();

    if (argc < 2) {
        std::cout << "Comandos: mine | chain | send | balance\n";
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "mine") {

        std::string miner = "miner1";
        if (argc >= 3) miner = argv[2];

        // 🔥 garante mineração sempre
        Transaction dummy;
        dummy.id = "tx_" + std::to_string(rand());

        TxOutput out;
        out.address = miner;
        out.amount = 1;

        dummy.outputs.push_back(out);

        mazechain.addTransaction(dummy);

        mazechain.minePendingTransactions(miner);

        Storage::saveChain(mazechain, "chain.dat");
    }

    else if (cmd == "chain") {

        for (const auto& block : mazechain.getChain()) {
            std::cout << "\n====================\n";
            std::cout << "Index: " << block.index << "\n";
            std::cout << "Hash: " << block.hash << "\n";
            std::cout << "Prev: " << block.previousHash << "\n";
        }
    }

    else if (cmd == "balance") {

        if (argc < 3) {
            std::cout << "Uso: balance <address>\n";
            return 0;
        }

        std::string addr = argv[2];

        std::cout << "💰 Saldo: " << mazechain.getBalance(addr) << "\n";
    }

    return 0;
}
