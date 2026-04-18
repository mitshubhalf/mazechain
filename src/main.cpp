#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include <iostream>
#include <sys/stat.h>
#include <string>
#include <iomanip>
#include <fstream>

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(path) _mkdir(path)
#else
    #include <sys/types.h>
    #define MKDIR(path) mkdir(path, 0777)
#endif

int main(int argc, char* argv[]) {
    // 1. Garante a pasta de dados
    MKDIR("data");

    // 2. Verificação da wordlist
    std::string wordlist_path = "wordlist.txt";
    std::ifstream check_wordlist(wordlist_path);

    if (!check_wordlist.is_open()) {
        check_wordlist.open("../wordlist.txt");

        if (check_wordlist.is_open()) {
            wordlist_path = "../wordlist.txt";
        } else {
            std::cerr << "❌ ERRO: wordlist.txt não encontrada!\n";
            std::cerr << "Execute:\n";
            std::cerr << "curl -L https://raw.githubusercontent.com/bitcoin/bips/master/bip-0039/english.txt -o wordlist.txt\n";
            return 1;
        }
    }
    check_wordlist.close();

    // 3. Inicializa blockchain
    Blockchain bc;

    if (!Storage::loadChain(bc, "data/blockchain.dat")) {
        std::cout << "[INFO] Nenhuma blockchain encontrada. Criando nova.\n";
    }

    // 4. Ajuda
    if (argc < 2) {
        std::cout << "==========================================\n";
        std::cout << "          MAZECHAIN CORE v2.1              \n";
        std::cout << "    ECONOMY: 20M MZ | UNIT: MITS          \n";
        std::cout << "==========================================\n";
        std::cout << "Uso: ./mazechain [comando] [argumentos]\n\n";
        std::cout << "Comandos:\n";
        std::cout << "  wallet create          - Gera nova carteira\n";
        std::cout << "  mine [endereco]        - Minera um bloco\n";
        std::cout << "  balance [endereco]     - Consulta saldo\n";
        std::cout << "  send [de] [para] [qtd] [seed]\n";
        std::cout << "  chain                  - Ver blocos\n";
        std::cout << "  stats                  - Ver supply\n";
        return 0;
    }

    std::string cmd = argv[1];

    // ---------------- WALLET ----------------
    if (cmd == "wallet" && argc > 2 && std::string(argv[2]) == "create") {
        Wallet w;
        w.create();

        std::cout << "------------------------------------------\n";
        std::cout << "✅ MAZECHAIN: CARTEIRA GERADA COM SUCESSO\n";
        std::cout << "ADDRESS: " << w.address << "\n";
        std::cout << "SEED   : " << w.seed << "\n";
        std::cout << "⚠️ Guarde sua SEED em lugar seguro!\n";
        std::cout << "------------------------------------------\n";
        return 0;
    }

    // ---------------- MINERAÇÃO ----------------
    else if (cmd == "mine" && argc > 2) {
        std::string minerAddress = argv[2];

        std::cout << "⛏️ Iniciando mineração para: " << minerAddress << "...\n";

        bc.mineBlock(minerAddress);

        // ✅ CORREÇÃO DEFINITIVA (SEM IF)
        Storage::saveChain(bc, "data/blockchain.dat");
        Storage::clearMempool("data/mempool.dat");

        std::cout << "✅ Bloco #" << (bc.getChain().size() - 1) << " minerado e salvo!\n";
        return 0;
    }

    // ---------------- BALANCE ----------------
    else if (cmd == "balance" && argc > 2) {
        double balance = bc.getBalance(argv[2]);

        std::cout << "------------------------------------------\n";
        std::cout << "💰 Endereço: " << argv[2] << "\n";
        std::cout << "💰 Saldo: " << std::fixed << std::setprecision(8) << balance << " MZ\n";
        std::cout << "💎 Mits:  " << (long long)(balance * 100000000.0) << " units\n";
        std::cout << "------------------------------------------\n";
        return 0;
    }

    // ---------------- SEND ----------------
    else if (cmd == "send" && argc > 5) {
        try {
            double amount = std::stod(argv[4]);

            bc.send(argv[2], argv[3], amount, argv[5]);

            std::cout << "✅ Transação enviada para a mempool.\n";

        } catch (const std::exception& e) {
            std::cerr << "❌ Erro: " << e.what() << "\n";
        }
        return 0;
    }

    // ---------------- CHAIN ----------------
    else if (cmd == "chain") {
        std::cout << "--- HISTÓRICO DA BLOCKCHAIN ---\n";

        for (const auto& b : bc.getChain()) {
            std::cout << "Bloco #" << b.index
                      << " | Hash: " << b.hash.substr(0,16) << "..."
                      << " | Txs: " << b.transactions.size() << "\n";
        }

        return 0;
    }

    // ---------------- STATS ----------------
    else if (cmd == "stats") {
        std::cout << "--- ESTATÍSTICAS DA REDE ---\n";
        std::cout << "Blocos: " << bc.getChain().size() << "\n";
        std::cout << "Circulação: " << std::fixed << std::setprecision(8)
                  << bc.getTotalSupply() << " MZ\n";
        return 0;
    }

    // ---------------- DEFAULT ----------------
    std::cout << "❓ Comando desconhecido. Use ./mazechain para ajuda.\n";
    return 0;
}
