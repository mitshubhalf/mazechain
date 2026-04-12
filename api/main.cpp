#include "crow_all.h"
#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include <vector>
#include <string>
#include <cstdlib>
#include <sstream>
#include <iostream>

// Função para habilitar comunicação com o Frontend (CORS)
void add_cors(crow::response& res) {
    res.add_header("Access-Control-Allow-Origin", "*");
    res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.add_header("Access-Control-Allow-Headers", "Content-Type");
}

int main() {
    crow::SimpleApp app;
    Blockchain bc;
    
    // Tenta carregar a chain existente do disco para não perder as moedas
    Storage::loadChain(bc, "data/blockchain.dat");

    // ROTA RAIZ - Verificação rápida de status
    CROW_ROUTE(app, "/")([]() {
        return "MAZECHAIN NODE v1.1 - STATUS: ONLINE";
    });

    // ROTA OPTIONS - Necessária para o navegador autorizar o envio de transações
    CROW_ROUTE(app, "/send").methods(crow::HTTPMethod::OPTIONS)([]() {
        crow::response res(204);
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");
        return res;
    });

    // ROTA DE ENVIO DE MOEDAS (POST)
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
                throw std::runtime_error("Campos obrigatorios ausentes no JSON");
            }

            std::string from = x["from"].s();
            std::string to = x["to"].s();
            double amount = x["amount"].d();
            std::string seed = x["seed"].s();

            // Envia para a lógica da Blockchain
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

    // ROTA PARA GERAR NOVA CARTEIRA
    CROW_ROUTE(app, "/wallet/new")([]() {
        Wallet w;
        w.create();
        std::stringstream ss;
        ss << "ENDERECO (MZ):\n" << w.address << "\n\nSEED:\n>> " << w.seed << " <<";
        crow::response res(ss.str());
        add_cors(res);
        return res;
    });

    // ROTA PARA CONSULTAR SALDO
    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string endereco) {
        double saldo = bc.getBalance(endereco);
        crow::json::wvalue x;
        x["address"] = endereco;
        x["balance"] = saldo;
        crow::response res{x};
        add_cors(res);
        return res;
    });

    // ROTA DE MINERAÇÃO
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
        Storage::saveChain(bc, "data/blockchain.dat"); // Salva no disco após minerar
        x["status"] = "success";
        x["blocos"] = (int)bc.getChain().size();
        crow::response res(x);
        add_cors(res);
        return res;
    });

    // ROTA PARA VER A BLOCKCHAIN COMPLETA
    CROW_ROUTE(app, "/chain")([&bc]() {
        std::vector<crow::json::wvalue> blocks_json;
        for (const auto& b : bc.getChain()) {
            crow::json::wvalue block;
            block["index"] = b.index;
            block["hash"] = b.hash;
            block["prevHash"] = b.prevHash;
            block["timestamp"] = b.timestamp;
            blocks_json.push_back(std::move(block));
        }
        crow::response res{crow::json::wvalue(blocks_json)};
        add_cors(res);
        return res;
    });

    // --- NOVAS FUNCIONALIDADES ---

    // ROTA PARA VER TRANSAÇÕES PENDENTES (MEMPOOL)
    CROW_ROUTE(app, "/mempool")([&bc]() {
        std::vector<crow::json::wvalue> txs_json;
        for (const auto& tx : bc.getMempool()) { 
            crow::json::wvalue j_tx;
            j_tx["from"] = tx.from;
            j_tx["to"] = tx.to;
            j_tx["amount"] = tx.amount;
            txs_json.push_back(std::move(j_tx));
        }
        crow::response res{crow::json::wvalue(txs_json)};
        add_cors(res);
        return res;
    });

    // ROTA DE STATUS GERAL DA REDE
    CROW_ROUTE(app, "/status")([&bc]() {
        crow::json::wvalue x;
        x["moeda"] = "MazeChain";
        x["simbolo"] = "MZ";
        x["total_blocos"] = (int)bc.getChain().size();
        x["mempool_size"] = (int)bc.getMempool().size();
        x["uptime"] = "ONLINE";
        
        crow::response res(x);
        add_cors(res);
        return res;
    });

    // CONFIGURAÇÃO DA PORTA DO RENDER
    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 10000;
    
    std::cout << "MazeChain Node rodando na porta " << port << std::endl;
    app.port(port).multithreaded().run();
}
