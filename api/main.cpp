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
#include <memory>

struct CORS {
    struct context {};
    void before_handle(crow::request& req, crow::response& res, context&) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PATCH, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Origin, Content-Type, Accept, Authorization");
        res.set_header("X-Content-Type-Options", "nosniff");
        res.set_header("X-Frame-Options", "DENY");
        
        if (req.method == crow::HTTPMethod::OPTIONS) {
            res.code = 204;
            res.end();
        }
    }
    void after_handle(crow::request&, crow::response& res, context&) {
        if(res.get_header_value("Access-Control-Allow-Origin").empty()) {
            res.set_header("Access-Control-Allow-Origin", "*");
        }
    }
};

int main() {
    std::ifstream check_wordlist("wordlist.txt");
    if (!check_wordlist.is_open()) {
        std::cerr << "CRITICAL ERROR: wordlist.txt missing!" << std::endl;
        return 1;
    }
    check_wordlist.close();

    crow::App<CORS> app;
    Blockchain bc;
    
    static std::string index_cache;
    std::ifstream file("index.html");
    if (file.is_open()) {
        index_cache.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
    }

    Storage::loadChain(bc, "data/blockchain.dat");

    CROW_ROUTE(app, "/")([]() {
        if (!index_cache.empty()) {
            crow::response res(index_cache);
            res.set_header("Content-Type", "text/html");
            return res;
        }
        return crow::response(200, "MAZECHAIN NODE v2.1.1 - [NODE RUNNING]");
    });

    CROW_ROUTE(app, "/status")([&bc]() {
        crow::json::wvalue x;
        x["network"]["status"] = "operational";
        x["network"]["version"] = "2.1.1-PROD";
        x["protocol"]["difficulty"] = bc.getDifficulty();
        x["protocol"]["total_supply"] = bc.getTotalSupply();
        x["protocol"]["max_supply"] = bc.getMaxSupply();
        x["chain"]["height"] = (int)bc.getChain().size();
        x["timestamp"] = std::time(nullptr);
        return crow::response(x);
    });

    CROW_ROUTE(app, "/send").methods(crow::HTTPMethod::POST)([&bc](const crow::request& req) {
        auto x = crow::json::load(req.body);
        crow::json::wvalue result;
        if (!x || !x.has("from") || !x.has("to") || !x.has("amount") || !x.has("seed")) {
            result["error"] = "Missing required fields";
            return crow::response(400, result);
        }
        try {
            bc.send(x["from"].s(), x["to"].s(), x["amount"].d(), x["seed"].s());
            result["status"] = "success";
            return crow::response(200, result);
        } catch (const std::exception& e) {
            result["status"] = "error";
            result["reason"] = e.what();
            return crow::response(400, result);
        }
    });

    CROW_ROUTE(app, "/chain")([&bc]() {
        crow::json::wvalue x;
        const auto& chain = bc.getChain();
        std::vector<crow::json::wvalue> block_list;
        size_t start = (chain.size() > 50) ? chain.size() - 50 : 0;
        for (size_t i = start; i < chain.size(); ++i) {
            const auto& block = chain[i];
            crow::json::wvalue b;
            b["index"] = block.index;
            b["hash"] = block.hash;
            b["prev_hash"] = block.prevHash;
            b["tx_count"] = (int)block.transactions.size();
            b["timestamp"] = block.timestamp;
            block_list.push_back(std::move(b));
        }
        x["blocks"] = std::move(block_list);
        x["tip_height"] = (int)chain.size();
        return crow::response(x);
    });

    CROW_ROUTE(app, "/wallet/new")([]() {
        Wallet w;
        w.create(); 
        crow::json::wvalue result;
        result["address"] = w.address;
        result["seed"] = w.seed;
        return crow::response(result);
    });

    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string endereco) {
        double bal = bc.getBalance(endereco);
        crow::json::wvalue x;
        x["address"] = endereco;
        x["balance_mz"] = bal;
        x["balance_mits"] = (long long)(bal * 100000000.0);
        return crow::response(x);
    });

    CROW_ROUTE(app, "/minerar_agora/<string>")([&bc](std::string endereco) {
        if (endereco.substr(0, 2) != "MZ") return crow::response(400, "Invalid Format");
        try {
            int current_height = (int)bc.getChain().size();
            bc.mineBlock(endereco);
            
            // CORREÇÃO TÉCNICA AQUI:
            Storage::saveChain(bc, "data/blockchain.dat");
            Storage::clearMempool("data/mempool.dat");

            crow::json::wvalue x;
            x["status"] = "success";
            x["block_mined"] = current_height;
            x["reward_claimed"] = bc.getBlockReward(current_height);
            x["new_height"] = (int)bc.getChain().size();
            return crow::response(x);
        } catch (const std::exception& e) {
            return crow::response(500, std::string("Mining failed: ") + e.what());
        }
    });

    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 10000;
    app.port(port).bindaddr("0.0.0.0").multithreaded().run();
}
