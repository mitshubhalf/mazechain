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
    res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.add_header("Access-Control-Allow-Headers", "Content-Type");
}

int main() {
    crow::SimpleApp app;
    Blockchain bc;
    
    // Tenta carregar a blockchain existente
    if(!Storage::loadChain(bc, "data/blockchain.dat")) {
        std::cout << "[INFO] Nenhuma blockchain encontrada ou erro na leitura. Iniciando nova." << std::endl;
    }

    // ROTA RAIZ
    CROW_ROUTE(app, "/")([]() {
        return "MAZECHAIN NODE v1.1.2 - STATUS: ONLINE";
    });

    // ROTA OPTIONS (CORS Preflight)
    CROW_ROUTE(app, "/send").methods(crow::HTTPMethod::OPTIONS)([]() {
        crow::response res(204);
        add_cors(res);
        return res;
    });

    // ROTA DE ENVIO - Protegida contra Double Spending na Mempool
    CROW_ROUTE(app, "/send").methods(crow::HTTPMethod::POST)([&bc](const crow::request& req) {
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

    // ROTA CARTEIRA
    CROW_ROUTE(app, "/wallet/new")([]() {
        Wallet w;
        w.create();
        std::stringstream ss;
        ss << "ENDERECO (MZ):\n" << w.address << "\n\nSEED:\n>> " << w.seed << " <<";
        crow::response res(ss.str());
        add_cors(res);
        return res;
    });

    // ROTA SALDO (Usa o UTXO Set atualizado)
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
        // Persistência imediata após minerar
        Storage::saveChain(bc, "data/blockchain.dat");
        bc.utxoSet.saveToFile("data/utxo.dat"); 
        
        x["status"] = "success";
        x["height"] = (int)bc.getChain().size();
        crow::response res(x);
        add_cors(res);
        return res;
    });

    // ROTA EXPLORADOR (Com suporte a paginação)
    CROW_ROUTE(app, "/chain")([&bc](const crow::request& req) {
        auto full_chain = bc.getChain();
        
        int limit = req.url_params.get("limit") ? std::stoi(req.url_params.get("limit")) : 50;
        int offset = req.url_params.get("offset") ? std::stoi(req.url_params.get("offset")) : 0;
        
        std::vector<crow::json::wvalue> blocks_json;
        int start = std::max(0, offset);
        int end = std::min((int)full_chain.size(), start + limit);

        for (int i = start; i < end; i++) {
            const auto& b = full_chain[i];
            crow::json::wvalue block;
            block["index"] = b.index;
            block["hash"] = b.hash;
            block["prevHash"] = b.prevHash;
            block["timestamp"] = (long long)b.timestamp;
            block["tx_count"] = (int)b.transactions.size();
            blocks_json.push_back(std::move(block));
        }
        
        crow::json::wvalue final_res;
        final_res["blocks"] = std::move(blocks_json);
        final_res["total"] = (int)full_chain.size();

        crow::response res{final_res};
        add_cors(res);
        return res;
    });

    // ROTA MEMPOOL
    CROW_ROUTE(app, "/mempool")([&bc]() {
        std::vector<crow::json::wvalue> txs_json;
        for (const auto& tx : bc.getMempool()) { 
            crow::json::wvalue j_tx;
            j_tx["id"] = tx.id;
            j_tx["signature"] = tx.signature;
            txs_json.push_back(std::move(j_tx));
        }
        crow::response res{crow::json::wvalue(txs_json)};
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

    // INICIALIZAÇÃO
    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 10000;
    
    std::cout << "==========================================" << std::endl;
    std::cout << "  MAZECHAIN NODE API - ONLINE PORTA " << port << std::endl;
    std::cout << "==========================================" << std::endl;
    
    app.port(port).multithreaded().run();
}
