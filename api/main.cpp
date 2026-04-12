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
    
    // Tenta carregar a chain existente do disco
    Storage::loadChain(bc, "data/blockchain.dat");

    CROW_ROUTE(app, "/")([]() {
        return "MAZECHAIN NODE v1.1 - STATUS: ONLINE";
    });

    // 1. ADICIONADO: ROTA OPTIONS PARA O NAVEGADOR
    // Sem isso, o frontend dá "Erro de Conexão" por causa do CORS
    CROW_ROUTE(app, "/send").methods(crow::HTTPMethod::OPTIONS)([]() {
        crow::response res(204);
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");
        return res;
    });

    // ROTA DE ENVIO
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
            // 2. MELHORIA: Verificação de existência das chaves no JSON para evitar crash
            if (!x.has("from") || !x.has("to") || !x.has("amount") || !x.has("seed")) {
                throw std::runtime_error("Campos obrigatorios ausentes no JSON");
            }

            std::string from = x["from"].s();
            std::string to = x["to"].s();
            double amount = x["amount"].d();
            std::string seed = x["seed"].s();

            // Chama a função de envio da classe Blockchain
            bc.send(from, to, amount, seed);

            result["status"] = "success";
            result["message"] = "Transacao enviada para Mempool";
            crow::response res(200, result);
            add_cors(res);
            return res;
        } catch (const std::exception& e) {
            result["status"] = "error";
            result["message"] = e.what();
            crow::response res(400, result); // 400 é mais apropriado para erro de lógica/saldo
            add_cors(res);
            return res;
        }
    });

    CROW_ROUTE(app, "/wallet/new")([]() {
        Wallet w;
        w.create();
        std::stringstream ss;
        ss << "ENDERECO (MZ):\n" << w.address << "\n\nSEED:\n>> " << w.seed << " <<";
        crow::response res(ss.str());
        add_cors(res);
        return res;
    });

    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string endereco) {
        double saldo = bc.getBalance(endereco);
        // Removido o erro 404 para carteiras zeradas, 
        // agora retorna saldo 0.0 (melhor para o frontend não bugar)
        crow::json::wvalue x;
        x["address"] = endereco;
        x["balance"] = saldo;
        crow::response res{x};
        add_cors(res);
        return res;
    });

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
        x["blocos"] = (int)bc.getChain().size();
        crow::response res(x);
        add_cors(res);
        return res;
    });

    CROW_ROUTE(app, "/chain")([&bc]() {
        std::vector<crow::json::wvalue> blocks_json;
        for (const auto& b : bc.getChain()) {
            crow::json::wvalue block;
            block["index"] = b.index;
            block["hash"] = b.hash;
            block["prevHash"] = b.prevHash;
            blocks_json.push_back(std::move(block));
        }
        crow::response res{crow::json::wvalue(blocks_json)};
        add_cors(res);
        return res;
    });

    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 10000;
    app.port(port).multithreaded().run();
}
