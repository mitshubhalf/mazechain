#include <iostream>
#include "blockchain.h"
#include "storage.h"

int main(int argc, char* argv[]) {

    Blockchain mazechain;

    // 🔥 carregar chain corretamente
    Storage::loadChain(mazechain, "chain.dat");

    // 🔥 reconstruir UTXO sempre após load
    mazechain.rebuildUTXO();

    if (argc < 2) {
        std::cout << "mine | chain | send | balance\n";
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "mine") {

        std::string miner = "miner1";
        if (argc >= 3) miner = argv[2];

        mazechain.minePendingTransactions(miner);

        std::cout << "✅ Bloco minerado com sucesso!\n";

        Storage::saveChain(mazechain, "chain.dat");
    }

    else if (cmd == "chain") {

        const auto& chain = mazechain.getChain();

        for (const auto& block : chain) {
            std::cout << "\n====================\n";
            std::cout << "Index: " << block.index << "\n";
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

        if (argc < 5) {
            std::cout << "Uso: send <from> <to> <amount>\n";
            return 0;
        }

        std::string from = argv[2];
        std::string to = argv[3];
        double amount = atof(argv[4]);

        if (mazechain.getBalance(from) < amount) {
            std::cout << "❌ Saldo insuficiente\n";
            return 0;
        }

        Transaction tx;
        tx.id = "tx_" + from + "_" + to + "_" + std::to_string(rand());

        TxOutput out;
        out.address = to;
        out.amount = amount;

        tx.outputs.push_back(out);

        mazechain.addTransaction(tx);

        std::cout << "✅ Transação criada\n";

        Storage::saveChain(mazechain, "chain.dat");
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
