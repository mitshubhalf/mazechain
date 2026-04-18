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

// Middleware de CORS aprimorado com Headers de Segurança
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
        // Garante que o header esteja presente mesmo após o processamento
        if(res.get_header_value("Access-Control-Allow-Origin").empty()) {
            res.set_header("Access-Control-Allow-Origin", "*");
        }
    }
};

int main() {
    // 1. Verificação de dependência crítica (BIP-39)
    std::ifstream check_wordlist("wordlist.txt");
    if (!check_wordlist.is_open()) {
        std::cerr << "CRITICAL ERROR: wordlist.txt missing! Cannot secure wallet generation." << std::endl;
        return 1;
    }
    check_wordlist.close();

    crow::App<CORS> app;
    Blockchain bc;
    
    // Cache do index.html para alta performance de leitura (In-Memory)
    static std::string index_cache;
    std::ifstream file("index.html");
    if (file.is_open()) {
        index_cache.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
    }

    // 2. Carregamento de Estado
    if(!Storage::loadChain(bc, "data/blockchain.dat")) {
        std::cout << ">>> [MAZECHAIN] No previous ledger found. Initializing genesis block..." << std::endl;
    }

    // --- ROTA: HOME (UI) ---
    CROW_ROUTE(app, "/")([]() {
        if (!index_cache.empty()) {
            crow::response res(index_cache);
            res.set_header("Content-Type", "text/html");
            return res;
        }
        return crow::response(200, "MAZECHAIN NODE v2.1.1 - [NODE RUNNING]");
    });

    // --- ROTA: STATUS DA REDE ---
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

    // --- ROTA: ENVIO DE TRANSAÇÃO ---
    CROW_ROUTE(app, "/send").methods(crow::HTTPMethod::POST)([&bc](const crow::request& req) {
        auto x = crow::json::load(req.body);
        crow::json::wvalue result;
        
        if (!x || !x.has("from") || !x.has("to") || !x.has("amount") || !x.has("seed")) {
            result["error"] = "Missing required fields: from, to, amount, seed";
            return crow::response(400, result);
        }

        try {
            std::string from = x["from"].s();
            std::string to = x["to"].s();
            double amount = x["amount"].d();
            std::string seed = x["seed"].s();

            // Validação básica de sanidade antes de processar
            if (amount <= 0) throw std::runtime_error("Amount must be greater than 0");
            if (from == to) throw std::runtime_error("Sender and Receiver cannot be the same");

            bc.send(from, to, amount, seed);
            
            std::cout << "[TX] New transaction from " << from.substr(0,10) << "... to mempool." << std::endl;

            result["status"] = "success";
            result["txid_request"] = std::time(nullptr); // Placeholder para ID de requisição
            return crow::response(200, result);
        } catch (const std::exception& e) {
            result["status"] = "error";
            result["reason"] = e.what();
            return crow::response(400, result);
        }
    });

    // --- ROTA: EXPLORER (HISTÓRICO) ---
    CROW_ROUTE(app, "/chain")([&bc]() {
        crow::json::wvalue x;
        const auto& chain = bc.getChain();
        std::vector<crow::json::wvalue> block_list;
        
        // Retorna apenas os últimos 50 blocos para evitar payload excessivo em chains grandes
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

    // --- ROTA: GERAÇÃO DE CARTEIRA (SECURE) ---
    CROW_ROUTE(app, "/wallet/new")([]() {
        Wallet w;
        w.create(); 
        
        crow::json::wvalue result;
        result["address"] = w.address;
        result["seed"] = w.seed;
        result["warning"] = "KEEP YOUR SEED SAFE. NO RECOVERY POSSIBLE.";
        return crow::response(result);
    });

    // --- ROTA: SALDO (ATUALIZAÇÃO EM TEMPO REAL) ---
    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string endereco) {
        double bal = bc.getBalance(endereco);
        crow::json::wvalue x;
        x["address"] = endereco;
        x["balance_mz"] = bal;
        x["balance_mits"] = (long long)(bal * 100000000.0);
        x["formatted"] = std::to_string(bal) + " MZ";
        return crow::response(x);
    });

    // --- ROTA: MINERAÇÃO (CONTROLADA) ---
    CROW_ROUTE(app, "/minerar_agora/<string>")([&bc](std::string endereco) {
        // Verificação de segurança de endereço
        if (endereco.substr(0, 2) != "MZ") {
            return crow::response(400, "Invalid MazeChain address format");
        }

        try {
            int current_height = (int)bc.getChain().size();
            
            std::cout << "[MINER] Mining request for: " << endereco << std::endl;
            bc.mineBlock(endereco);

            // Persistência Atômica
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

    // --- CONFIGURAÇÃO DE PORTA DINÂMICA (CLOUD FRIENDLY) ---
    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 10000;
    
    std::cout << "===============================================" << std::endl;
    std::cout << " MAZECHAIN NODE v2.1.1 IS RUNNING ON PORT " << port << std::endl;
    std::cout << " BIND ADDRESS: 0.0.0.0 (OPEN FOR EXTERNAL) " << std::endl;
    std::cout << "===============================================" << std::endl;

    app.port(port).bindaddr("0.0.0.0").multithreaded().run();
}
