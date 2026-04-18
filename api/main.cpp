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

// Estrutura de Middleware para comunicação com o Frontend
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

// --- FUNÇÃO PRINCIPAL COM SUPORTE A CLI (TERMINAL) E API (WEB) ---
int main(int argc, char* argv[]) {
    // 1. Verificação da wordlist (Sempre necessária)
    std::ifstream check_wordlist("wordlist.txt");
    if (!check_wordlist.is_open()) {
        std::cerr << "❌ ERRO CRÍTICO: wordlist.txt não encontrada!" << std::endl;
        return 1;
    }
    check_wordlist.close();

    Blockchain bc;
    
    // Carrega a chain existente do disco antes de qualquer operação
    if(!Storage::loadChain(bc, "data/blockchain.dat")) {
        std::cout << "[INFO] Nenhuma blockchain local encontrada. Iniciando nova rede..." << std::endl;
    }

    // --- MODO TERMINAL (CLI) ---
    // Se você digitar: ./mazechain mine [endereco]
    if (argc > 1) {
        std::string cmd = argv[1];

        if (cmd == "mine" && argc > 2) {
            std::string endereco = argv[2];
            std::cout << "⛏️  Iniciando mineração via terminal para: " << endereco << std::endl;
            
            int altura_atual = (int)bc.getChain().size();
            bc.mineBlock(endereco);

            // Salva o progresso no disco após minerar
            Storage::saveChain(bc, "data/blockchain.dat");
            Storage::clearMempool("data/mempool.dat");

            std::cout << "✅ Bloco #" << altura_atual << " minerado com sucesso!" << std::endl;
            return 0; // Encerra o programa após minerar
        }

        if (cmd == "wallet" && argc > 2 && std::string(argv[2]) == "create") {
            Wallet w;
            w.create();
            std::cout << "\n✅ MAZECHAIN: CARTEIRA GERADA" << std::endl;
            std::cout << "ADDRESS: " << w.address << std::endl;
            std::cout << "SEED   : " << w.seed << std::endl;
            return 0;
        }
    }

    // --- MODO SERVIDOR (API CROW) ---
    // Se você rodar apenas: ./mazechain
    crow::App<CORS> app; 

    CROW_ROUTE(app, "/")([]() {
        std::ifstream file("index.html");
        if (file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            crow::response res(content);
            res.set_header("Content-Type", "text/html; charset=UTF-8");
            return res;
        }
        return crow::response(200, "MAZECHAIN NODE v2.1 - STATUS: ONLINE");
    });

    CROW_ROUTE(app, "/status")([&bc]() {
        crow::json::wvalue x;
        x["status"] = "online";
        x["version"] = "2.1.0";
        x["blocks"] = (int)bc.getChain().size();
        return crow::response(x);
    });

    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string endereco) {
        double bal = bc.getBalance(endereco);
        crow::json::wvalue x;
        x["address"] = endereco;
        x["balance_mz"] = bal;
        return crow::response(x);
    });

    CROW_ROUTE(app, "/minerar_agora/<string>")([&bc](std::string endereco) {
        bc.mineBlock(endereco);
        Storage::saveChain(bc, "data/blockchain.dat");
        Storage::clearMempool("data/mempool.dat");
        return crow::response(200, "OK");
    });

    // Configuração de porta para Render/Replit
    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 10000;
    
    std::cout << "🚀 MazeChain API rodando na porta " << port << "..." << std::endl;
    app.port(port).bindaddr("0.0.0.0").multithreaded().run();
}
