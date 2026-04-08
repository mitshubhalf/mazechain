#include <iostream>
#include <string>
#include "blockchain.h"
#include "transaction.h"
#include "storage.h"

int main(int argc, char* argv[]) {

    Blockchain mazechain;

    // 🔥 CARREGA SEMPRE AO INICIAR
    Storage::loadChain(mazechain, "data/chain.txt");

    if (argc < 2) {
        std::cout << "Comandos disponíveis:\n";
        std::cout << "  mine\n";
        std::cout << "  chain\n";
        std::cout << "  send FROM TO AMOUNT\n";
        std::cout << "  balance ADDRESS\n";
        return 0;
    }

    std::string command = argv[1];

    // ⛏️ MINERAR
    if (command == "mine") {

        std::cout << "⛏️ Mining...\n";

        mazechain.minePendingTransactions("miner1");

        std::cout << "✅ Block mined!\n";

        Storage::saveChain(mazechain, "data/chain.txt");
    }

    // 📦 MOSTRAR CHAIN
    else if (command == "chain") {

        const auto& chain = mazechain.getChain();

        for (const auto& block : chain) {
            std::cout << "\nIndex: " << block.index << "\n";
            std::cout << "Hash: " << block.hash << "\n";
            std::cout << "Prev: " << block.previousHash << "\n";

            // 🔥 MOSTRAR TRANSAÇÕES
            for (const auto& tx : block.transactions) {
                std::cout << "  Tx: "
                          << tx.from << " -> "
                          << tx.to << " : "
                          << tx.amount << "\n";
            }
        }
    }

    // 💸 ENVIAR TRANSAÇÃO
    else if (command == "send") {

        if (argc < 5) {
            std::cout << "Uso: send FROM TO AMOUNT\n";
            return 0;
        }

        std::string from = argv[2];
        std::string to = argv[3];

        double amount;

        try {
            amount = std::stod(argv[4]);
        } catch (...) {
            std::cout << "❌ Valor inválido!\n";
            return 0;
        }

        if (from.empty() || to.empty()) {
            std::cout << "❌ Endereço inválido!\n";
            return 0;
        }

        // 🔥 ADD TRANSAÇÃO
        mazechain.addTransaction(Transaction(from, to, amount));

        std::cout << "✅ Transação adicionada\n";

        // 🔥 🔥 🔥 MINERA NA HORA (ESSA ERA A FALHA ANTES)
        std::cout << "⛏️ Mining...\n";
        mazechain.minePendingTransactions("miner1");
        std::cout << "✅ Block mined!\n";

        // 🔥 SALVA TUDO
        Storage::saveChain(mazechain, "data/chain.txt");
    }

    // 💰 VER SALDO
    else if (command == "balance") {

        if (argc < 3) {
            std::cout << "Uso: balance ADDRESS\n";
            return 0;
        }

        std::string addr = argv[2];

        if (addr.empty()) {
            std::cout << "❌ Endereço inválido!\n";
            return 0;
        }

        double balance = mazechain.getBalance(addr);

        std::cout << "Saldo: " << balance << "\n";
    }

    else {
        std::cout << "❌ Comando desconhecido\n";
    }

    return 0;
}
