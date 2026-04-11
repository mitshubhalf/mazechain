#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include <iostream>
#include <sys/stat.h>
#include <string>
#include <iomanip>

int main(int argc, char* argv[]) {
    #ifdef _WIN32
        mkdir("data");
    #else
        mkdir("data", 0777);
    #endif

    Blockchain bc;
    Storage::loadChain(bc, "data/blockchain.dat");

    if (argc < 2) {
        std::cout << "==========================================\n";
        std::cout << "          MAZECHAIN CORE v2.0             \n";
        std::cout << "==========================================\n";
        std::cout << "Usage: ./mazechain [comando] [argumentos]\n\n";
        std::cout << "Comandos disponíveis:\n";
        std::cout << "  wallet create          - Gera uma nova carteira\n";
        std::cout << "  mine [endereco]        - Minera um bloco\n";
        std::cout << "  balance [endereco]     - Consulta o saldo\n";
        std::cout << "  send [de] [para] [qtd] [seed] - Envia moedas (Requer Seed)\n";
        std::cout << "  mempool                - Lista transações pendentes\n";
        std::cout << "  chain                  - Exibe a blockchain\n";
        std::cout << "  verify                 - Valida a rede\n";
        std::cout << "  stats                  - Estatísticas\n";
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "wallet" && argc > 2) {
        if (std::string(argv[2]) == "create") {
            Wallet w;
            w.create();
        }
    }
    else if (cmd == "mine" && argc > 2) {
        bc.mineBlock(argv[2]);
    }
    else if (cmd == "balance" && argc > 2) {
        std::cout << "💰 Saldo: " << std::fixed << std::setprecision(8) << bc.getBalance(argv[2]) << " MZ" << std::endl;
    }
    // CORREÇÃO: Agora aceita 5 argumentos (./mazechain send de para qtd seed)
    else if (cmd == "send" && argc > 5) {
        bc.send(argv[2], argv[3], std::stod(argv[4]), argv[5]);
    }
    else if (cmd == "mempool") {
        auto pending = Storage::loadMempool("data/mempool.dat");
        std::cout << "📦 Transações na Mempool: " << pending.size() << std::endl;
    }
    else if (cmd == "chain") {
        for (const auto& b : bc.getChain()) {
            std::cout << "Bloco #" << b.index << " | Hash: " << b.hash << "\n";
        }
    }
    else if (cmd == "verify") {
        if (bc.isChainValid()) std::cout << "✅ Integridade OK!" << std::endl;
    }
    else if (cmd == "stats") {
        bc.printStats();
    }
    return 0;
}
