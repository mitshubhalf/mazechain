#include "../include/blockchain.h"
#include "../include/storage.h"
#include <iostream>
#include <sys/stat.h>

int main(int argc, char* argv[]) {
    // Cria a pasta data se ela não existir
    #ifdef _WIN32
        mkdir("data");
    #else
        mkdir("data", 0777);
    #endif

    Blockchain bc;

    // Carrega a blockchain existente
    Storage::loadChain(bc, "data/blockchain.dat");

    if (argc < 2) {
        std::cout << "Usage: ./mazechain [command] [args]\n";
        std::cout << "Commands: mine <address> | balance <address> | send <from> <to> <amount>\n";
        return 0;
    }

    std::string cmd = argv[1];

    try {
        if (cmd == "mine" && argc > 2) {
            bc.mineBlock(argv[2]);
        }
        else if (cmd == "balance" && argc > 2) {
            std::cout << "💰 Balance of " << argv[2] << ": " << bc.getBalance(argv[2]) << "\n";
        }
        else if (cmd == "send" && argc > 4) {
            bc.send(argv[2], argv[3], std::stod(argv[4]));
        }
        else {
            std::cout << "❌ Comando inválido ou argumentos faltando.\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Erro: " << e.what() << "\n";
    }

    return 0;
}
