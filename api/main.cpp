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

// Função para habilitar comunicação com o Frontend (CORS)
void add_cors(crow::response& res) {
    res.add_header("Access-Control-Allow-Origin", "*");
    res.add_header("Access-Control-Allow-Methods", "GET, POST, PATCH, PUT, DELETE, OPTIONS");
    res.add_header("Access-Control-Allow-Headers", "Origin, Content-Type, Accept, Authorization");
}

int main() {
    crow::SimpleApp app;
    Blockchain bc;
    
    // Tenta carregar a blockchain existente
    if(!Storage::loadChain(bc, "data/blockchain.dat")) {
        std::cout << "[INFO] Nenhuma blockchain encontrada. Iniciando nova." << std::endl;
    }

    // ROTA RAIZ
    CROW_ROUTE(app, "/")([]() {
        return "MAZECHAIN NODE v1.1.2 - STATUS: ONLINE";
    });

    // --- ROTA DE ENVIO (POST + OPTIONS) ---
    CROW_ROUTE(app, "/send").methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)([&bc](const crow::request& req) {
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
            std::string seed = x["seed"].s();

            // Lógica de saldo pendente na Mempool
            double pendingSpend = 0;
            for (const auto& tx : bc.getMempool()) {
                for (const auto& out : tx.vout) {
                    if (out.address == from) pendingSpend += std::abs(out.amount);
                }
            }

            double balance = bc.getBalance(from) - pendingSpend;
            if (balance < (amount * 1.01)) {
                result["status"] = "error";
                result["message"] = "Saldo insuficiente ou pendente na mempool";
                crow::response res(402, result);
                add_cors(res);
                return res;
            }

            bc.send(from, to, amount, seed);
            result["status"] = "success";
            result["message"] = "Transacao enviada com sucesso";
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

    // --- NOVA ROTA CHAIN (Para listar blocos) ---
    CROW_ROUTE(app, "/chain")([&bc]() {
        crow::json::wvalue x;
        x["length"] = (int)bc.getChain().size();
        // Aqui você pode expandir para retornar os blocos se o método existir
        crow::response res(x);
        add_cors(res);
        return res;
    });

    // --- NOVA ROTA STATS (Estatísticas da rede) ---
    CROW_ROUTE(app, "/stats")([&bc]() {
        crow::json::wvalue x;
        x["height"] = (int)bc.getChain().size();
        x["supply"] = bc.getTotalSupply();
        x["mempool_size"] = (int)bc.getMempool().size();
        x["difficulty"] = bc.getDifficulty();
        crow::response res(x);
        add_cors(res);
        return res;
    });

    // ROTA CARTEIRA NOVA
    CROW_ROUTE(app, "/wallet/new")([]() {
        Wallet w;
        w.create();
        crow::json::wvalue result;
        result["address"] = w.address;
        result["seed"] = w.seed;
        crow::response res(result);
        add_cors(res);
        return res;
    });

    // ROTA SALDO
    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string endereco) {
        crow::json::wvalue x;
        x["balance"] = bc.getBalance(endereco);
        crow::response res(x);
        add_cors(res);
        return res;
    });

    // ROTA MINERAÇÃO
    CROW_ROUTE(app, "/minerar_agora/<string>")([&bc](std::string endereco) {
        bc.mineBlock(endereco);
        Storage::saveChain(bc, "data/blockchain.dat");
        crow::json::wvalue x;
        x["status"] = "success";
        crow::response res(x);
        add_cors(res);
        return res;
    });

    // Inicialização na porta do Render
    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 10000;
    app.port(port).multithreaded().run();
}
