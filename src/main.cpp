#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include <iostream>
#include <sys/stat.h>
#include <string>

int main(int argc, char* argv[]) {
    // Cria pasta de dados se não existir
    #ifdef _WIN32
        mkdir("data");
    #else
        mkdir("data", 0777);
    #endif

    Blockchain bc;
    Storage::loadChain(bc, "data/blockchain.dat");

    if (argc < 2) {
        std::cout << "Uso: ./mazechain [mine/balance/send/wallet/chain/verify/stats]\n";
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "wallet" && argc > 2) {
        if (std::string(argv[2]) == "create") {
            Wallet w; 
            w.create(); // Chama a lógica corrigida
        }
    }
    else if (cmd == "mine" && argc > 2) {
        bc.mineBlock(argv[2]);
    }
    else if (cmd == "balance" && argc > 2) {
        std::cout << "💰 Saldo: " << bc.getBalance(argv[2]) << " MZ" << std::endl;
    }
    else if (cmd == "send" && argc > 4) {
        bc.send(argv[2], argv[3], std::stod(argv[4]));
    }
    else if (cmd == "verify") {
        if (bc.isChainValid()) std::cout << "✅ Blockchain íntegra!" << std::endl;
        else std::cout << "🚨 Erro de integridade!" << std::endl;
    }
    else if (cmd == "stats") {
        bc.printStats();
    }
    else if (cmd == "chain") {
        for (const auto& b : bc.getChain()) {
            std::cout << "Bloco #" << b.index << " | Hash: " << b.hash.substr(0,10) << "..." << std::endl;
        }
    }

    return 0;
}
