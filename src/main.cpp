#include <iostream>
#include "blockchain.h"

int main(int argc, char* argv[]) {

    Blockchain mazechain;

    if (argc < 2) {
        std::cout << "mine | balance | send\n";
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "mine") {

        std::cout << "⛏️ Mining...\n";
        mazechain.minePendingTransactions("miner1");
        std::cout << "✅ Block mined!\n";
    }

    else if (cmd == "balance") {

        std::string addr = argv[2];
        std::cout << "Saldo: " << mazechain.getBalance(addr) << "\n";
    }

    else if (cmd == "send") {

        std::string from = argv[2];
        std::string to = argv[3];
        double amount = atof(argv[4]);

        if (mazechain.getBalance(from) < amount) {
            std::cout << "❌ Saldo insuficiente!\n";
            return 0;
        }

        Transaction tx;
        tx.id = "tx_" + from + "_" + to;

        TxInput in;
        in.txId = "coinbase";
        in.outputIndex = 0;
        in.address = from;

        TxOutput out;
        out.address = to;
        out.amount = amount;

        tx.inputs.push_back(in);
        tx.outputs.push_back(out);

        mazechain.addTransaction(tx);

        std::cout << "✅ Transação enviada\n";
    }

    return 0;
}
