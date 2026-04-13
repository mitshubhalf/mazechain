#include "../include/crow_all.h"
#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include "../include/transaction.h"
#include <vector>
#include <string>
#include <cstdlib>
#include <iostream>

// Função global para injetar CORS em todas as respostas
void add_cors(crow::response& res) {
    res.add_header("Access-Control-Allow-Origin", "*");
    res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

int main() {
    crow::SimpleApp app;
    Blockchain bc;
    
    Storage::loadChain(bc, "data/blockchain.dat");

    // 1. ROTA DE ENVIO (POST + OPTIONS para CORS)
    CROW_ROUTE(app, "/send").methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)([&bc](const crow::request& req) {
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204);
            add_cors(res);
            return res;
        }

        auto x = crow::json::load(req.body);
        crow::json::wvalue result;
        if (!x) {
            crow::response res(400, "JSON Invalido");
            add_cors(res);
            return res;
        }

        try {
            std::string from = x["from"].s();
            std::string to = x["to"].s();
            double amount = x["amount"].d();
            std::string seed = x["seed"].s();

            bc.send(from, to, amount, seed);
            result["status"] = "success";
            crow::response res(200, result);
            add_cors(res);
            return res;
        } catch (const std::exception& e) {
            result["status"] = "error";
            result["message"] = e.what();
            crow::response res(400, result);
            add_cors(res);
            return res;
        }
    });

    // 2. ROTA DA CHAIN (GET)
    CROW_ROUTE(app, "/chain")([&bc]() {
        crow::json::wvalue x;
        // Se você tiver um método que retorna o tamanho da chain ou os blocos:
        x["length"] = (int)bc.getChain().size();
        crow::response res(x);
        add_cors(res);
        return res;
    });

    // 3. ROTA DE SALDO (GET)
    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string addr) {
        crow::json::wvalue x;
        x["balance"] = bc.getBalance(addr);
        crow::response res(x);
        add_cors(res);
        return res;
    });

    // 4. ROTA DE MINERAÇÃO (GET)
    CROW_ROUTE(app, "/minerar_agora/<string>")([&bc](std::string addr) {
        bc.mineBlock(addr);
        Storage::saveChain(bc, "data/blockchain.dat");
        crow::json::wvalue x;
        x["status"] = "success";
        crow::response res(x);
        add_cors(res);
        return res;
    });

    // 5. ROTA STATUS/STATS (GET)
    CROW_ROUTE(app, "/status")([&bc]() {
        crow::json::wvalue x;
        x["height"] = (int)bc.getChain().size();
        x["supply"] = bc.getTotalSupply();
        crow::response res(x);
        add_cors(res);
        return res;
    });

    // Configuração de Porta para o Render
    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 10000;
    app.port(port).multithreaded().run();
}
