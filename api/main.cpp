#define CROW_MAIN
#define CROW_ENABLE_CORS
#include "crow_all.h"
#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include "../include/transaction.h"
#include <vector>
#include <string>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sys/stat.h>

struct CORS {
    struct context {};
    void before_handle(crow::request& req, crow::response& res, context&) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PATCH, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Origin, Content-Type, Accept, Authorization");
        if (req.method == crow::HTTPMethod::OPTIONS) {
            res.code = 204;
            res.end();
        }
    }
    void after_handle(crow::request&, crow::response& res, context&) {
        res.set_header("Access-Control-Allow-Origin", "*");
    }
};

int main(int argc, char* argv[]) {
    // 1. Garantir que a pasta 'data' existe (na raiz do projeto)
    #ifdef _WIN32
        mkdir("data");
    #else
        mkdir("data", 0777);
    #endif

    // 2. Verificação inteligente da wordlist.txt
    std::string wl = "wordlist.txt";
    std::ifstream check_wl(wl);
    if (!check_wl.is_open()) {
        check_wl.open("../wordlist.txt"); // Tenta um nível acima se estiver na pasta build
        if (check_wl.is_open()) wl = "../wordlist.txt";
        else {
            std::cerr << "❌ ERRO: wordlist.txt não encontrada! Baixe-a com o comando curl sugerido anteriormente." << std::endl;
            return 1;
        }
    }
    check_wl.close();

    Blockchain bc;
    
    // 3. Carregar a Chain (Caminho relativo à raiz)
    if(!Storage::loadChain(bc, "data/blockchain.dat")) {
        std::cout << "[SISTEMA] Nenhuma blockchain local encontrada. Iniciando nova rede..." << std::endl;
    }

    // --- MODO TERMINAL (CLI) ---
    if (argc > 1) {
        std::string cmd = argv[1];

        // COMANDO: wallet create
        if (cmd == "wallet" && argc > 2 && std::string(argv[2]) == "create") {
            Wallet w;
            w.create(); // A função create() agora usará a 'wl' correta internamente ou o padrão
            std::cout << "\n✅ MAZECHAIN: CARTEIRA GERADA" << std::endl;
            std::cout << "ADDRESS: " << w.address << std::endl;
            std::cout << "SEED   : " << w.seed << std::endl;
            std::cout << "------------------------------------------" << std::endl;
            return 0; // Finaliza para não abrir o servidor
        }

        // COMANDO: mine [endereco]
        if (cmd == "mine" && argc > 2) {
            std::string miner_addr = argv[2];
            std::cout << "⛏️  Minerando bloco via terminal para: " << miner_addr << "..." << std::endl;
            
            bc.mineBlock(miner_addr);
            Storage::saveChain(bc, "data/blockchain.dat");
            Storage::clearMempool("data/mempool.dat");

            std::cout << "✅ Sucesso! Bloco #" << (bc.getChain().size() - 1) << " minerado." << std::endl;
            return 0; // Finaliza para não abrir o servidor
        }
    }

    // --- MODO SERVIDOR (API CROW) ---
    crow::App<CORS> app; 

    CROW_ROUTE(app, "/")([]() {
        std::ifstream file("index.html");
        if (file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            crow::response res(content);
            res.set_header("Content-Type", "text/html; charset=UTF-8");
            return res;
        }
        return crow::response(200, "MAZECHAIN NODE v2.1 - ONLINE");
    });

    CROW_ROUTE(app, "/status")([&bc]() {
        crow::json::wvalue x;
        x["blocks"] = (int)bc.getChain().size();
        x["supply"] = bc.getTotalSupply();
        return x;
    });

    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string endereco) {
        crow::json::wvalue x;
        x["address"] = endereco;
        x["balance"] = bc.getBalance(endereco);
        return x;
    });

    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 10000;
    
    std::cout << "🚀 MazeChain API Online na porta " << port << "..." << std::endl;
    app.port(port).bindaddr("0.0.0.0").multithreaded().run();
}
