#include "../include/crow_all.h"
#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include "../include/transaction.h"
#include <vector>
#include <string>
#include <cstdlib>
#include <iostream>

int main() {
    crow::SimpleApp app;
    Blockchain bc;
    
    // Carrega dados persistentes
    Storage::loadChain(bc, "data/blockchain.dat");

    // --- MIDDLEWARE DE CORS GLOBAL ---
    // Isso garante que TODA resposta do servidor tenha os headers necessários
    app.loglevel(crow::LogLevel::Debug);

    // ROTA RAIZ (Teste de vida)
    CROW_ROUTE(app, "/")([]() {
        return "MAZECHAIN NODE ONLINE";
    });

    // ROTA DE STATUS (Para verificar se o servidor responde)
    CROW_ROUTE(app, "/status")([&bc]() {
        crow::json::wvalue x;
        x["status"] = "online";
        x["blocks"] = (int)bc.getChain().size();
        crow::response res(x);
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });

    // ROTA DE ENVIO (POST + OPTIONS)
    CROW_ROUTE(app, "/send").methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)([&bc](const crow::request& req) {
        crow::response res;
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");

        if (req.method == crow::HTTPMethod::OPTIONS) {
            res.code = 204;
            return res;
        }

        auto x = crow::json::load(req.body);
        if (!x) {
            res.code = 400;
            res.body = "{\"status\":\"error\",\"message\":\"JSON Invalido\"}";
            return res;
        }

        try {
            std::string from = x["from"].s();
            std::string to = x["to"].s();
            double amount = x["amount"].d();
            std::string seed = x["seed"].s();

            bc.send(from, to, amount, seed);
            
            res.code = 200;
            res.body = "{\"status\":\"success\"}";
            return res;
        } catch (const std::exception& e) {
            res.code = 400;
            crow::json::wvalue err;
            err["status"] = "error";
            err["message"] = e.what();
            res.body = crow::json::dump(err);
            return res;
        }
    });

    // ROTA DE SALDO
    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string addr) {
        crow::json::wvalue x;
        x["balance"] = bc.getBalance(addr);
        crow::response res(x);
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });

    // ROTA MINERAR
    CROW_ROUTE(app, "/minerar_agora/<string>")([&bc](std::string addr) {
        bc.mineBlock(addr);
        Storage::saveChain(bc, "data/blockchain.dat");
        crow::json::wvalue x;
        x["status"] = "success";
        crow::response res(x);
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });

    // ROTA CARTEIRA NOVA
    CROW_ROUTE(app, "/wallet/new")([]() {
        Wallet w;
        w.create();
        crow::json::wvalue x;
        x["address"] = w.address;
        x["seed"] = w.seed;
        crow::response res(x);
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });

    // ROTA IMPORTAR
    CROW_ROUTE(app, "/wallet/import").methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)([](const crow::request& req) {
        crow::response res;
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");

        if (req.method == crow::HTTPMethod::OPTIONS) {
            res.code = 204;
            return res;
        }

        auto x = crow::json::load(req.body);
        Wallet w;
        w.fromSeed(x["seed"].s());
        crow::json::wvalue res_json;
        res_json["address"] = w.address;
        res.body = crow::json::dump(res_json);
        return res;
    });

    // Configuração de porta para Render
    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 10000;
    app.port(port).multithreaded().run();
}
