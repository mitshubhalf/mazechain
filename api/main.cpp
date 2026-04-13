#include "../include/crow_all.h"
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

// Função Centralizada de CORS - Garante que o Navegador aceite a resposta
void add_cors(crow::response& res) {
    res.add_header("Access-Control-Allow-Origin", "*");
    res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

int main() {
    crow::SimpleApp app;
    Blockchain bc;
    
    if(!Storage::loadChain(bc, "data/blockchain.dat")) {
        std::cout << "[INFO] Iniciando nova blockchain." << std::endl;
    }

    // ROTA RAIZ
    CROW_ROUTE(app, "/")([]() {
        return "MAZECHAIN NODE ONLINE";
    });

    // ROTA DE ENVIO (Onde está dando o erro de CORS)
    CROW_ROUTE(app, "/send").methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)([&bc](const crow::request& req) {
        // 1. Responde ao "Preflight" do navegador
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204);
            add_cors(res);
            return res;
        }

        auto x = crow::json::load(req.body);
        crow::json::wvalue result;
        
        if (!x) {
            result["status"] = "error";
            result["message"] = "JSON Invalido";
            crow::response res(400, result);
            add_cors(res);
            return res;
        }

        try {
            std::string from = x["from"].s();
            std::string to = x["to"].s();
            double amount = x["amount"].d();
            std::string seed = x["seed"].s(); // A "Assinatura" é validada aqui via Seed

            // Lógica de saldo pendente na Mempool para evitar Double Spend
            double pendingSpend = 0;
            for (const auto& tx : bc.getMempool()) {
                for (const auto& out : tx.vout) {
                    if (out.address == from) pendingSpend += std::abs(out.amount);
                }
            }

            double balance = bc.getBalance(from) - pendingSpend;
            if (balance < (amount * 1.01)) { // Valor + 1% taxa
                result["status"] = "error";
                result["message"] = "Saldo insuficiente (considerando pendentes)";
                crow::response res(402, result);
                add_cors(res);
                return res;
            }

            // O método send() do seu backend deve usar a seed para assinar internamente
            bc.send(from, to, amount, seed);

            result["status"] = "success";
            result["message"] = "Transacao em processamento (Mempool)";
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

    // ROTA SALDO (GET)
    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string endereco) {
        crow::json::wvalue x;
        x["balance"] = bc.getBalance(endereco);
        crow::response res(x);
        add_cors(res);
        return res;
    });

    // ROTA MINERAR
    CROW_ROUTE(app, "/minerar_agora/<string>")([&bc](std::string endereco) {
        bc.mineBlock(endereco);
        Storage::saveChain(bc, "data/blockchain.dat");
        crow::json::wvalue x;
        x["status"] = "success";
        crow::response res(x);
        add_cors(res);
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
        add_cors(res);
        return res;
    });

    // ROTA IMPORTAR
    CROW_ROUTE(app, "/wallet/import").methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)([](const crow::request& req) {
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204);
            add_cors(res);
            return res;
        }
        auto x = crow::json::load(req.body);
        Wallet w;
        w.fromSeed(x["seed"].s());
        crow::json::wvalue res_json;
        res_json["address"] = w.address;
        crow::response res(res_json);
        add_cors(res);
        return res;
    });

    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 10000;
    app.port(port).multithreaded().run();
}
