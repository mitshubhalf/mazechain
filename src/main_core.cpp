#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include "../include/transaction.h"
#include "../include/p2p.h"      // Nova inclusão para P2P
#include "../include/httplib.h"  // Nova inclusão para Servidor HTTP
#include <iostream>
#include <sys/stat.h>
#include <string>
#include <iomanip>
#include <fstream>
#include <vector>
#include <thread>                // Para rodar o servidor em paralelo
#include <future>

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(path) _mkdir(path)
#else
    #include <sys/types.h>
    #define MKDIR(path) mkdir(path, 0777)
#endif

// DEFINIÇÃO DOS CAMINHOS ABSOLUTOS
const std::string ABS_DATA_PATH = "/home/runner/workspace/data";
const std::string ABS_DB_FILE = "/home/runner/workspace/data/blockchain.dat";
const std::string ABS_MEMPOOL_FILE = "/home/runner/workspace/data/mempool.dat";

void print_full_usage() {
    std::cout << "==========================================\n";
    std::cout << "         MAZECHAIN CORE v3.0 (P2P)        \n";
    std::cout << "    ECONOMY: 20M MZ | UNIT: MITS          \n";
    std::cout << "==========================================\n";
    std::cout << "Uso: ./mazechain [comando] [argumentos]\n\n";
    std::cout << "Comandos de Carteira:\n";
    std::cout << "  wallet create                - Gera nova carteira\n";
    std::cout << "  wallet from-seed \"<seed>\"    - Recupera endereço\n";
    std::cout << "\nComandos de Rede:\n";
    std::cout << "  mine [endereco]              - Minera um bloco\n";
    std::cout << "  balance [endereco]           - Consulta saldo\n";
    std::cout << "  send [de] [para] [qtd] [seed]- Envia MZ tokens\n";
    std::cout << "  mempool                      - Lista pendentes\n";
    std::cout << "\nComandos de Blockchain:\n";
    std::cout << "  chain                        - Histórico de blocos\n";
    std::cout << "  chain stats                  - Supply e estatísticas\n";
    std::cout << "  chain validate               - Valida integridade\n";
    std::cout << "  p2p add [url]                - Adiciona um peer (ex: http://ip:8080)\n";
    std::cout << "==========================================\n";
}

int main(int argc, char* argv[]) {
    // 1. Garante a pasta de dados
    MKDIR(ABS_DATA_PATH.c_str());

    // 2. Verificação da wordlist
    std::string wordlist_path = "wordlist.txt";
    std::ifstream check_wordlist(wordlist_path);
    if (!check_wordlist.is_open()) {
        check_wordlist.open("../wordlist.txt");
        if (check_wordlist.is_open()) wordlist_path = "../wordlist.txt";
        else {
            std::cerr << "❌ ERRO: wordlist.txt não encontrada!\n";
            return 1;
        }
    }
    check_wordlist.close();

    // 3. Inicializa componentes Core
    Blockchain bc;
    P2P p2p;

    // ---------------- INICIALIZAÇÃO DO SERVIDOR P2P (BACKGROUND) ----------------
    httplib::Server svr;

    // Rota para outros nós enviarem blocos para você
    svr.Post("/block", [&](const httplib::Request& req, httplib::Response& res) {
        std::cout << "\n📡 [REDE] Novo bloco recebido via P2P..." << std::endl;
        // Lógica de validação e adição de bloco via JSON seria processada aqui
        res.set_content("Recebido", "text/plain");
    });

    // Rota para receber transações de outros nós
    svr.Post("/tx", [&](const httplib::Request& req, httplib::Response& res) {
        std::cout << "\n📡 [REDE] Nova transação recebida via P2P..." << std::endl;
        res.set_content("Recebida", "text/plain");
    });

    // Inicia o servidor em uma thread separada
    std::thread server_thread([&]() {
        svr.listen("0.0.0.0", 8080);
    });
    server_thread.detach(); 
    // ----------------------------------------------------------------------------

    if (argc < 2) {
        print_full_usage();
        return 0;
    }

    std::string cmd = argv[1];

    // ---------------- COMANDOS P2P ----------------
    if (cmd == "p2p") {
        if (argc >= 4 && std::string(argv[2]) == "add") {
            p2p.add_peer(argv[3]);
            return 0;
        }
    }

    // ---------------- COMANDOS WALLET ----------------
    if (cmd == "wallet") {
        if (argc < 3) return 1;
        std::string sub = argv[2];
        if (sub == "create") {
            Wallet w; w.create();
            std::cout << "------------------------------------------\n";
            std::cout << "✅ CARTEIRA GERADA: " << w.address << "\n";
            std::cout << "SEED: " << w.seed << "\n";
            std::cout << "------------------------------------------\n";
            return 0;
        } 
        else if (sub == "from-seed") {
            std::string full_seed = "";
            for (int i = 3; i < argc; i++) {
                if (i > 3) full_seed += " ";
                full_seed += argv[i];
            }
            Wallet w; w.fromSeed(full_seed);
            std::cout << "\n✅ ADDRESS: " << w.address << "\n";
            return 0;
        }
    }

    // ---------------- COMANDO MINE ----------------
    else if (cmd == "mine" && argc > 2) {
        std::string minerAddress = argv[2];
        std::cout << "⛏️ Preparando mineração do bloco #" << bc.getChain().size() << "...\n";

        if (!bc.isChainValid()) return 1;

        bc.mineBlock(minerAddress);

        Storage::saveChain(bc, ABS_DB_FILE);
        Storage::clearMempool(ABS_MEMPOOL_FILE);

        // BROADCAST: Avisa a rede que você minerou um bloco
        p2p.broadcast_block("{\"status\": \"new_block_mined\"}");

        std::cout << "✅ Operação concluída e propagada.\n";
        return 0;
    }

    // ---------------- COMANDO BALANCE ----------------
    else if (cmd == "balance" && argc > 2) {
        double balance = bc.getBalance(argv[2]);
        std::cout << "------------------------------------------\n";
        std::cout << "💰 Endereço: " << argv[2] << "\n";
        std::cout << std::fixed << std::setprecision(8);
        std::cout << "💰 Saldo:    " << balance << " MZ\n";
        std::cout << "💎 Mits:     " << (long long)(balance * 100000000.0) << " units\n";
        std::cout << "------------------------------------------\n";
        return 0;
    }

    // ---------------- COMANDO SEND ----------------
    else if (cmd == "send" && argc > 5) {
        try {
            double amount = std::stod(argv[4]);
            bc.send(argv[2], argv[3], amount, argv[5]);

            // BROADCAST: Avisa a rede sobre a nova transação
            p2p.broadcast_transaction("{\"info\": \"new_tx\"}");

            std::cout << "✅ Transação enviada para o mempool e rede.\n";
        } catch (const std::exception& e) {
            std::cerr << "❌ Erro: " << e.what() << "\n";
            return 1;
        }
        return 0;
    }

    // ---------------- COMANDO MEMPOOL ----------------
    else if (cmd == "mempool") {
        auto pending = Storage::loadMempool(ABS_MEMPOOL_FILE);
        if (pending.empty()) std::cout << "Mempool vazia.\n";
        else {
            for (const auto& tx : pending) {
                std::cout << "ID: " << tx.id.substr(0,10) << " | Valor: " << tx.vout[0].amount << " MZ\n";
            }
        }
        return 0;
    }

    // ---------------- COMANDOS CHAIN ----------------
    else if (cmd == "chain") {
        std::string sub = (argc > 2) ? argv[2] : "";
        if (sub == "stats") bc.printStats();
        else if (sub == "validate") {
            if (bc.isChainValid()) std::cout << "✅ Blockchain íntegra.\n";
            else std::cout << "❌ Integridade comprometida!\n";
        } else {
            for (const auto& b : bc.getChain()) {
                std::cout << "Bloco #" << b.index << " | Hash: " << b.hash.substr(0,16) << "...\n";
            }
        }
        return 0;
    }

    return 0;
}