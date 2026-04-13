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
// Essencial para evitar o erro "Erro de conexão com o Node" no navegador
void add_cors(crow::response& res) {
    res.add_header("Access-Control-Allow-Origin", "*");
    res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.add_header("Access-Control-Allow-Headers", "Content-Type");
}

int main() {
    crow::SimpleApp app;
    Blockchain bc;
    
    // Tenta carregar a blockchain existente e o conjunto UTXO
    if(!Storage::loadChain(bc, "data/blockchain.dat")) {
        std::cout << "[INFO] Nenhuma blockchain encontrada ou erro na leitura. Iniciando nova." << std::endl;
    }

    // ROTA RAIZ
    CROW_ROUTE(app, "/")([]() {
        return "MAZECHAIN NODE v1.1.2 - STATUS: ONLINE";
    });

    // ROTA OPTIONS (CORS Preflight global)
    // O navegador envia um OPTIONS antes de qualquer POST
    CROW_ROUTE(app, "/<string>").methods(crow::HTTPMethod::OPTIONS)([](std::string path) {
        crow::response res(204);
        add_cors(res);
        return res;
    });

    // ROTA DE ENVIO - Protegida contra Double Spending na Mempool
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
            if (!x.has("from") || !x.has("to") || !x.has("amount") || !x.has("seed")) {
                throw std::runtime_error("Campos obrigatorios ausentes");
            }

            std::string from = x["from"].s();
            std::string to = x["to"].s();
            double amount = x["amount"].d();
            std::string seed = x["seed"].s();

            double totalNeeded = amount * 1.01; // Valor + 1% de taxa
            
            // --- CÁLCULO DE SALDO PENDENTE (MEMPOOL) ---
            double pendingSpend = 0;
            std::vector<Transaction> mempool = bc.getMempool();
            for (const auto& tx : mempool) {
                for (const auto& out : tx.vout) {
                    if (out.amount < 0 && out.address == from) {
                        pendingSpend += std::abs(out.amount);
                    }
                }
            }

            double currentBalance = bc.getBalance(from);
            double effectiveBalance = currentBalance - pendingSpend;

            if (effectiveBalance < (totalNeeded - 0.000001)) {
                result["status"] = "error";
                result["message"] = "Saldo insuficiente (pendente na mempool)";
                result["available_now"] = effectiveBalance;
                result["needed"] = totalNeeded;
                crow::response res(402, result);
                add_cors(res);
                return res;
            }

            bc.send(from, to, amount, seed);

            result["status"] = "success";
            result["message"] = "Transacao enviada para Mempool";
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

    // ROTA CARTEIRA (CORRIGIDA PARA JSON)
    // O Frontend espera um objeto JSON para salvar no localStorage
    CROW_ROUTE(app, "/wallet/new")([]() {
        Wallet w;
        w.create();
        
        crow::json::wvalue result;
        result["status"] = "success";
        result["address"] = w.address;
        result["seed"] = w.seed;

        crow::response res(result);
        add_cors(res);
        return res;
    });

    // ROTA IMPORTAR CARTEIRA (NOVA)
    CROW_ROUTE(app, "/wallet/import").methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)([](const crow::request& req) {
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204);
            add_cors(res);
            return res;
        }

        auto x = crow::json::load(req.body);
        crow::json::wvalue result;

        if (!x || !x.has("seed")) {
            result["status"] = "error";
            result["message"] = "Seed nao fornecida";
            crow::response res(400, result);
            add_cors(res);
            return res;
        }

        Wallet w;
        std::string seed = x["seed"].s();
        // A lógica w.fromSeed deve estar implementada no seu wallet.cpp
        w.fromSeed(seed); 

        result["status"] = "success";
        result["address"] = w.address;
        
        crow::response res(result);
        add_cors(res);
        return res;
    });

    // ROTA SALDO
    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string endereco) {
        double saldo = bc.getBalance(endereco);
        crow::json::wvalue x;
        x["address"] = endereco;
        x["balance"] = saldo;
        crow::response res{x};
        add_cors(res);
        return res;
    });

    // ROTA MINERAÇÃO
    CROW_ROUTE(app, "/minerar_agora/<string>")([&bc](std::string endereco) {
        crow::json::wvalue x;
        if(endereco.substr(0, 2) != "MZ") {
            x["status"] = "error";
            x["message"] = "Endereco invalido";
            crow::response res(400, x);
            add_cors(res);
            return res;
        }

        bc.mineBlock(endereco);
        Storage::saveChain(bc, "data/blockchain.dat");
        
        x["status"] = "success";
        x["height"] = (int)bc.getChain().size();
        x["message"] = "Bloco minerado com sucesso";
        
        crow::response res(x);
        add_cors(res);
        return res;
    });

    // ROTA STATUS
    CROW_ROUTE(app, "/status")([&bc]() {
        crow::json::wvalue x;
        x["moeda"] = "MazeCoin";
        x["simbolo"] = "MZ";
        x["versao"] = "1.1.2";
        x["total_blocos"] = (int)bc.getChain().size();
        x["circulacao_total"] = bc.getTotalSupply();
        x["dificuldade"] = bc.getDifficulty();
        
        crow::response res(x);
        add_cors(res);
        return res;
    });

    // INICIALIZAÇÃO NA PORTA DINÂMICA (RENDER)
    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 10000;
    
    std::cout << "==========================================" << std::endl;
    std::cout << "  MAZECHAIN NODE API - ONLINE PORTA " << port << std::endl;
    std::cout << "==========================================" << std::endl;
    
    app.port(port).multithreaded().run();
}
