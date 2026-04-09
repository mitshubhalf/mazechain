#include <iostream>
#include <cstdlib>
#include "blockchain.h"
#include "storage.h"

int main(int argc, char* argv[]) {

    Blockchain mazechain;

    // 🔥 carregar chain salva
    Storage::loadChain(mazechain, "chain.dat");

    if (argc < 2) {
        std::cout << "Comandos: mine | chain | send | balance\n";
        return 0;
    }

    std::string cmd = argv[1];

    // =========================
    // ⛏️ MINERAR
    // =========================
    if (cmd == "mine") {

        std::cout << "⛏️ Iniciando mineração...\n";

        std::string miner = "miner1";
        if (argc >= 3) miner = argv[2];

        // 🔥 se não tiver transação, cria uma dummy
        if (mazechain.getChain().size() > 0) {
            Transaction dummy;
            dummy.id = "dummy_tx_" + std::to_string(rand());

            mazechain.addTransaction(dummy);
        }

        mazechain.minePendingTransactions(miner);

        std::cout << "✅ Bloco minerado com sucesso!\n";

        // 💾 salvar após mineração
        Storage::saveChain(mazechain, "chain.dat");
    }

    // =========================
    // 📦 VER CHAIN
    // =========================
    else if (cmd == "chain") {

        const auto& chain = mazechain.getChain();

        for (const auto& block : chain) {
            std::cout << "\n====================\n";
            std::cout << "Index: " << block.index << "\n";
            std::cout << "Hash: " << block.hash << "\n";
            std::cout << "Prev: " << block.previousHash << "\n";

            for (const auto& tx : block.transactions) {
                std::cout << "Tx ID: " << tx.id << "\n";

                for (const auto& out : tx.outputs) {
                    std::cout << "  -> " << out.address << " : " << out.amount << "\n";
                }
            }
        }
    }

    // =========================
    // 💸 ENVIAR
    // =========================
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
        tx.id = "tx_" + from + "_" + to;

        TxOutput out;
        out.address = to;
        out.amount = amount;

        tx.outputs.push_back(out);

        mazechain.addTransaction(tx);

        std::cout << "✅ Transação criada\n";

        Storage::saveChain(mazechain, "chain.dat");
    }

    // =========================
    // 💰 BALANCE
    // =========================
    else if (cmd == "balance") {

        if (argc < 3) {
            std::cout << "Uso: balance <address>\n";
            return 0;
        }

        std::string addr = argv[2];

        std::cout << "💰 Saldo: " << mazechain.getBalance(addr) << "\n";
    }

    else {
        std::cout << "Comando inválido\n";
    }

    return 0;
}
