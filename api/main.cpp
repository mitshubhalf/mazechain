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

int main() {
    // Verificação de segurança: a wordlist é essencial para o funcionamento das rotas de wallet
    std::ifstream check_wordlist("wordlist.txt");
    if (!check_wordlist.is_open()) {
        std::cerr << "❌ ERRO: wordlist.txt nao encontrada! O servidor nao pode iniciar sem a lista BIP-39." << std::endl;
        std::cerr << "Execute: curl -L https://raw.githubusercontent.com/bitcoin/bips/master/bip-0039/english.txt -o wordlist.txt" << std::endl;
        return 1;
    }
    check_wordlist.close();

    crow::App<CORS> app; 
    Blockchain bc;
    
    // Carrega a chain existente ou inicia uma nova
    if(!Storage::loadChain(bc, "data/blockchain.dat")) {
        std::cout << "[INFO] Iniciando nova rede MazeChain..." << std::endl;
    }

    CROW_ROUTE(app, "/")([]() {
        std::ifstream file("index.html");
        if (file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            crow::response res(content);
            res.set_header("Content-Type", "text/html");
            return res;
        }
        return crow::response(200, "MAZECHAIN NODE v2.1 - STATUS: ONLINE (20M MAX SUPPLY)");
    });

    // --- STATUS COMPLETO ---
    CROW_ROUTE(app, "/status")([&bc]() {
        crow::json::wvalue x;
        x["status"] = "online";
        x["version"] = "2.1.0";
        x["difficulty"] = bc.getDifficulty();
        x["total_supply"] = bc.getTotalSupply();
        x["max_supply"] = 20000000.0;
        x["blocks"] = (int)bc.getChain().size();
        return crow::response(x);
    });

    // --- ENVIO COM TAXA DE 1% ---
    CROW_ROUTE(app, "/send").methods(crow::HTTPMethod::POST)([&bc](const crow::request& req) {
        auto x = crow::json::load(req.body);
        crow::json::wvalue result;
        if (!x) return crow::response(400, "JSON Invalido");

        try {
            std::string from = x["from"].s();
            std::string to = x["to"].s();
            double amount = x["amount"].d();
            std::string seed = x["seed"].s();

            // O bc.send já valida internamente a assinatura e o saldo
            bc.send(from, to, amount, seed);
            
            result["status"] = "success";
            result["message"] = "Transacao em analise na mempool";
            return crow::response(200, result);
        } catch (const std::exception& e) {
            result["status"] = "error";
            result["message"] = e.what();
            return crow::response(400, result);
        }
    });

    // --- EXPLORER DE BLOCOS ---
    CROW_ROUTE(app, "/chain")([&bc]() {
        crow::json::wvalue x;
        std::vector<crow::json::wvalue> block_list;
        for (const auto& block : bc.getChain()) {
            crow::json::wvalue b;
            b["index"] = block.index;
            b["hash"] = block.hash;
            b["prev_hash"] = block.prevHash;
            b["merkle_root"] = block.merkleRoot;
            b["tx_count"] = (int)block.transactions.size();
            block_list.push_back(std::move(b));
        }
        x["blocks"] = std::move(block_list);
        return crow::response(x);
    });

    // --- NOVA CARTEIRA (BIP-39 - 12 PALAVRAS) ---
    CROW_ROUTE(app, "/wallet/new")([]() {
        Wallet w;
        w.create(); // Isso agora gera 12 palavras usando o wordlist.txt
        crow::json::wvalue result;
        result["address"] = w.address;
        result["seed"] = w.seed;
        return crow::response(result);
    });

    // --- CONSULTA DE SALDO EM MITS ---
    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string endereco) {
        double bal = bc.getBalance(endereco);
        crow::json::wvalue x;
        x["address"] = endereco;
        x["balance_mz"] = bal;
        x["balance_mits"] = (long long)(bal * 100000000);
        return crow::response(x);
    });

    // --- MINERAÇÃO VIA WEB ---
    CROW_ROUTE(app, "/minerar_agora/<string>")([&bc](std::string endereco) {
        int current_height = (int)bc.getChain().size();
        
        // 1. Minera o bloco (Executa o Proof of Work)
        bc.mineBlock(endereco);

        // 2. Persiste os dados (Salva no disco)
        Storage::saveChain(bc, "data/blockchain.dat");
        Storage::clearMempool("data/mempool.dat");

        crow::json::wvalue x;
        x["status"] = "success";
        x["block"] = current_height;
        x["reward"] = bc.getBlockReward(current_height);
        return crow::response(x);
    });

    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 10000;
    app.port(port).bindaddr("0.0.0.0").multithreaded().run();
}
