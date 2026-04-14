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

// Função para habilitar comunicação total com o Frontend (CORS)
// Atualizada para ser mais rigorosa com o Preflight do Navegador
void add_cors(crow::response& res) {
    res.add_header("Access-Control-Allow-Origin", "*");
    res.add_header("Access-Control-Allow-Methods", "GET, POST, PATCH, PUT, DELETE, OPTIONS");
    res.add_header("Access-Control-Allow-Headers", "Origin, Content-Type, Accept, Authorization, X-Requested-With");
    res.add_header("Access-Control-Max-Age", "3600");
}

int main() {
    crow::SimpleApp app;
    Blockchain bc;
    
    // Tenta carregar a blockchain existente
    if(!Storage::loadChain(bc, "data/blockchain.dat")) {
        std::cout << "[INFO] Nenhuma blockchain encontrada. Iniciando nova." << std::endl;
    }

    // --- ROTA RAIZ ---
    CROW_ROUTE(app, "/")([]() {
        return "MAZECHAIN NODE v1.1.2 - STATUS: ONLINE";
    });

    // --- ROTA DE STATUS ---
    CROW_ROUTE(app, "/status")([&bc]() {
        crow::json::wvalue x;
        x["status"] = "online";
        x["version"] = "1.1.2";
        x["difficulty"] = bc.getDifficulty();
        x["total_supply"] = bc.getTotalSupply();
        x["total_blocks"] = (int)bc.getChain().size();
        crow::response res(x);
        add_cors(res);
        return res;
    });

    // --- ROTA DE ENVIO (CORREÇÃO DEFINITIVA DO CORS) ---
    CROW_ROUTE(app, "/send").methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)([&bc](const crow::request& req) {
        crow::response res;
        add_cors(res);

        // Se for OPTIONS (Preflight), responde imediatamente com 204 (No Content)
        if (req.method == crow::HTTPMethod::OPTIONS) {
            res.code = 204;
            return res;
        }

        auto x = crow::json::load(req.body);
        crow::json::wvalue result;
        
        if (!x) {
            result["status"] = "error";
            result["message"] = "JSON Invalido";
            res.code = 400;
            res.body = result.dump();
            return res;
        }

        try {
            std::string from = x["from"].s();
            std::string to = x["to"].s();
            double amount = x["amount"].d();
            std::string seed = x["seed"].s();

            // Cálculo de saldo considerando a Mempool
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
                res.code = 402;
                res.body = result.dump();
                return res;
            }

            bc.send(from, to, amount, seed);
            result["status"] = "success";
            result["message"] = "Transacao enviada para a mempool";
            res.code = 200;
            res.body = result.dump();
            return res;

        } catch (const std::exception& e) {
            result["status"] = "error";
            result["message"] = e.what();
            res.code = 400;
            res.body = result.dump();
            return res;
        }
    });

    // --- ROTA CHAIN ---
    CROW_ROUTE(app, "/chain")([&bc]() {
        crow::json::wvalue x;
        std::vector<crow::json::wvalue> block_list;
        
        // Percorre a blockchain e envia detalhes extras para a Lupa de busca
        for (const auto& block : bc.getChain()) {
            crow::json::wvalue b;
            b["index"] = block.index;
            b["hash"] = block.hash;
            b["nonce"] = block.nonce;
            b["prev_hash"] = block.previous_hash;
            b["transactions_count"] = (int)block.transactions.size();
            block_list.push_back(std::move(b));
        }
        
        x["blocks"] = std::move(block_list);
        x["length"] = (int)bc.getChain().size();
        
        crow::response res(x);
        add_cors(res);
        return res;
    });

    // --- ROTA CARTEIRA NOVA ---
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
    
    // --- ROTA IMPORTAR CARTEIRA ---
    CROW_ROUTE(app, "/wallet/import").methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)([](const crow::request& req) {
        crow::response res;
        add_cors(res);

        if (req.method == crow::HTTPMethod::OPTIONS) {
            res.code = 204;
            return res;
        }

        auto x = crow::json::load(req.body);
        crow::json::wvalue result;

        if (!x || !x.has("seed")) {
            result["status"] = "error";
            result["message"] = "Seed obrigatoria";
            res.code = 400;
            res.body = result.dump();
            return res;
        }

        std::string userSeed = x["seed"].s();
        Wallet w;
        w.fromSeed(userSeed); 

        result["address"] = w.address;
        result["status"] = "success";
        res.code = 200;
        res.body = result.dump();
        return res;
    });

    // --- ROTA SALDO ---
    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string endereco) {
        crow::json::wvalue x;
        x["balance"] = bc.getBalance(endereco);
        crow::response res(x);
        add_cors(res);
        return res;
    });

    // --- ROTA MINERAÇÃO (ATUALIZADA PARA 1000 MZ NO LOG) ---
    CROW_ROUTE(app, "/minerar_agora/<string>")([&bc](std::string endereco) {
        bc.mineBlock(endereco);
        Storage::saveChain(bc, "data/blockchain.dat");
        
        crow::json::wvalue x;
        x["status"] = "success";
        x["height"] = (int)bc.getChain().size();
        x["reward"] = 1000; // Define o valor da recompensa para o Front ler
        
        crow::response res(x);
        add_cors(res);
        return res;
    });

    // --- CONFIGURAÇÃO DE PORTA DINÂMICA ---
    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 8080;

    std::cout << "==========================================" << std::endl;
    std::cout << "MAZECHAIN NODE ATIVO NA PORTA: " << port << std::endl;
    std::cout << "ESCUTANDO EM: 0.0.0.0 (Global)" << std::endl;
    std::cout << "==========================================" << std::endl;

    app.port(port).bindaddr("0.0.0.0").multithreaded().run();
}
