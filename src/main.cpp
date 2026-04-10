#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include <iostream>
#include <sys/stat.h>
#include <string>

int main(int argc, char* argv[]) {
    #ifdef _WIN32
        mkdir("data");
    #else
        mkdir("data", 0777);
    #endif

    Blockchain bc;
    Storage::loadChain(bc, "data/blockchain.dat");

    if (argc < 2) {
        std::cout << "Usage: ./mazechain [mine/balance/send/wallet/chain/mempool/block/verify]\n";
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "wallet" && argc > 2) {
        if (std::string(argv[2]) == "create") {
            std::string mnem = Wallet::generateMnemonic();
            std::cout << "📝 Seed: " << mnem << "\n💳 Addr: " << Wallet::deriveAddress(mnem) << std::endl;
        }
    }
    else if (cmd == "mine" && argc > 2) {
        bc.mineBlock(argv[2]);
    }
    else if (cmd == "balance" && argc > 2) {
        std::cout << "💰 Balance: " << bc.getBalance(argv[2]) << " MZ" << std::endl;
    }
    else if (cmd == "send" && argc > 4) {
        bc.send(argv[2], argv[3], std::stod(argv[4]));
    }
    else if (cmd == "mempool") {
        auto pending = Storage::loadMempool("data/mempool.dat");
        std::cout << "📦 Transações pendentes: " << pending.size() << std::endl;
    }
    else if (cmd == "chain") {
        for (const auto& b : bc.getChain()) {
            std::cout << "Bloco #" << b.index << " | Hash: " << b.hash.substr(0,10) << " | TXs: " << b.transactions.size() << std::endl;
        }
    }
    else if (cmd == "block" && argc > 2) {
        try {
            int height = std::stoi(argv[2]);
            bc.printBlockDetails(height);
        } catch (...) {
            std::cout << "❌ Erro: Forneça um número de bloco válido." << std::endl;
        }
    }
    // <-- NOVO: Comando de Verificação -->
    else if (cmd == "verify") {
        if (bc.isChainValid()) {
            std::cout << "✅ Blockchain íntegra! Todos os blocos estão validados." << std::endl;
        } else {
            std::cout << "🚨 ALERTA: Blockchain corrompida ou fraudada detectada!" << std::endl;
        }
    }

    return 0;
}
