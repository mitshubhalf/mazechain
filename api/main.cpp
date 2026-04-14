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

// =======================
// 🔥 MIDDLEWARE CORS GLOBAL
// =======================
struct CORS {
    struct context {};

    void before_handle(crow::request& req, crow::response& res, context&) {
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Origin, Content-Type, Accept, Authorization");

        if (req.method == crow::HTTPMethod::OPTIONS) {
            res.code = 204;
            res.end();
        }
    }

    void after_handle(crow::request&, crow::response& res, context&) {
        res.add_header("Access-Control-Allow-Origin", "*");
    }
};

int main() {

    crow::App<CORS> app; // 🔥 IMPORTANTE: usando middleware

    Blockchain bc;
    
    if(!Storage::loadChain(bc, "data/blockchain.dat")) {
        std::cout << "[INFO] Nenhuma blockchain encontrada. Iniciando nova." << std::endl;
    }

    // --- ROTA RAIZ ---
    CROW_ROUTE(app, "/")([]() {
        return "MAZECHAIN NODE v1.1.2 - STATUS: ONLINE";
    });

    // --- STATUS ---
    CROW_ROUTE(app, "/status")([&bc]() {
        crow::json::wvalue x;
        x["status"] = "online";
        x["version"] = "1.1.2";
        x["difficulty"] = bc.getDifficulty();
        x["total_supply"] = bc.getTotalSupply();
        x["total_blocks"] = (int)bc.getChain().size();
        return crow::response(x);
    });

    // --- SEND (SEM GAMBIARRA) ---
    CROW_ROUTE(app, "/send").methods(crow::HTTPMethod::POST)([&bc](const crow::request& req) {

        auto x = crow::json::load(req.body);
        crow::json::wvalue result;

        if (!x) {
            result["status"] = "error";
            result["message"] = "JSON Invalido";
            return crow::response(400, result.dump());
        }

        try {
            std::string from = x["from"].s();
            std::string to = x["to"].s();
            double amount = x["amount"].d();
            std::string seed = x["seed"].s();

            double pendingSpend = 0;
            for (const auto& tx : bc.getMempool()) {
                for (const auto& out : tx.vout) {
                    if (out.address == from) pendingSpend += std::abs(out.amount);
                }
            }

            double balance = bc.getBalance(from) - pendingSpend;

            if (balance < (amount * 1.01)) {
                result["status"] = "error";
                result["message"] = "Saldo insuficiente ou pendente";
                return crow::response(402, result.dump());
            }

            bc.send(from, to, amount, seed);

            result["status"] = "success";
            result["message"] = "Transacao enviada para a mempool";

            return crow::response(200, result.dump());

        } catch (const std::exception& e) {
            result["status"] = "error";
            result["message"] = e.what();
            return crow::response(400, result.dump());
        }
    });

    // --- CHAIN ---
    CROW_ROUTE(app, "/chain")([&bc]() {
        crow::json::wvalue x;
        std::vector<crow::json::wvalue> block_list;
        
        for (const auto& block : bc.getChain()) {
            crow::json::wvalue b;
            b["index"] = block.index;
            b["hash"] = block.hash;
            b["nonce"] = block.nonce;
            b["prev_hash"] = block.prevHash;
            b["transactions_count"] = (int)block.transactions.size();
            block_list.push_back(std::move(b));
        }
        
        x["blocks"] = std::move(block_list);
        x["length"] = (int)bc.getChain().size();

        return crow::response(x);
    });

    // --- WALLET NEW ---
    CROW_ROUTE(app, "/wallet/new")([]() {
        Wallet w;
        w.create();

        crow::json::wvalue result;
        result["address"] = w.address;
        result["seed"] = w.seed;

        return crow::response(result);
    });

    // --- WALLET IMPORT ---
    CROW_ROUTE(app, "/wallet/import").methods(crow::HTTPMethod::POST)([](const crow::request& req) {

        auto x = crow::json::load(req.body);
        crow::json::wvalue result;

        if (!x || !x.has("seed")) {
            result["status"] = "error";
            result["message"] = "Seed obrigatoria";
            return crow::response(400, result.dump());
        }

        std::string userSeed = x["seed"].s();
        Wallet w;
        w.fromSeed(userSeed);

        result["address"] = w.address;
        result["status"] = "success";

        return crow::response(200, result.dump());
    });

    // --- BALANCE ---
    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string endereco) {
        crow::json::wvalue x;
        x["balance"] = bc.getBalance(endereco);
        return crow::response(x);
    });

    // --- MINERAR ---
    CROW_ROUTE(app, "/minerar_agora/<string>")([&bc](std::string endereco) {
        bc.mineBlock(endereco);
        Storage::saveChain(bc, "data/blockchain.dat");

        crow::json::wvalue x;
        x["status"] = "success";
        x["height"] = (int)bc.getChain().size();
        x["reward"] = 1000;

        return crow::response(x);
    });

    // --- PORTA ---
    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 8080;

    std::cout << "==========================================" << std::endl;
    std::cout << "MAZECHAIN NODE ATIVO NA PORTA: " << port << std::endl;
    std::cout << "ESCUTANDO EM: 0.0.0.0 (Global)" << std::endl;
    std::cout << "==========================================" << std::endl;

    app.port(port).bindaddr("0.0.0.0").multithreaded().run();
}
