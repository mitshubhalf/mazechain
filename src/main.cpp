#include <iostream>
#include "blockchain.h"

int main(int argc, char* argv[]) {

    Blockchain mazechain;

    if (argc < 2) {
        std::cout << "mine | chain | send | balance\n";
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "mine") {

        std::cout << "⛏️ Mining...\n";
        mazechain.minePendingTransactions("miner1");
        std::cout << "✅ Block mined!\n";
    }

    else if (cmd == "chain") {

        const auto& chain = mazechain.getChain();

        for (const auto& block : chain) {
            std::cout << "\nIndex: " << block.index << "\n";
            std::cout << "Hash: " << block.hash << "\n";

            for (const auto& tx : block.transactions) {
                std::cout << "Tx ID: " << tx.id << "\n";

                for (const auto& out : tx.outputs) {
                    std::cout << "  -> " << out.address << " : " << out.amount << "\n";
                }
            }
        }
    }

    else if (cmd == "send") {

        std::string from = argv[2];
        std::string to = argv[3];
        double amount = atof(argv[4]);

        if (mazechain.getBalance(from) < amount) {
            std::cout << "❌ Saldo insuficiente\n";
            return 0;
        }

        Transaction tx;
        tx.id = "tx_" + from + "_" + to;

        TxOutput out;
        out.address = to;
        out.amount = amount;

        tx.outputs.push_back(out);

        mazechain.addTransaction(tx);

        std::cout << "✅ Transação criada\n";
    }

    else if (cmd == "balance") {

        std::string addr = argv[2];
        std::cout << "Saldo: " << mazechain.getBalance(addr) << "\n";
    }

    return 0;
}
