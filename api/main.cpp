#include "crow_all.h"
#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include <vector>
#include <string>
#include <cstdlib>
#include <set>
#include <sstream>
#include <iostream>

std::set<std::string> peers;

// Função auxiliar para injetar CORS em todas as respostas
void add_cors(crow::response& res) {
    res.add_header("Access-Control-Allow-Origin", "*");
    res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.add_header("Access-Control-Allow-Headers", "Content-Type");
}

int main() {
    crow::SimpleApp app;
    Blockchain bc;
    Storage::loadChain(bc, "data/blockchain.dat");

    CROW_ROUTE(app, "/")([]() {
        return "MAZECHAIN NODE v1.0 - STATUS: ONLINE";
    });

    // MELHORIA: Retorna JSON para o frontend capturar com precisão
    CROW_ROUTE(app, "/wallet/new")([]() {
        Wallet w;
        w.create();
        
        crow::json::wvalue x;
        x["address"] = w.address;
        x["seed"] = w.seed;

        // O frontend agora vai ler: >> {seed} << e MZ... automaticamente
        // Mas o JSON é mais seguro. Vamos manter o formato que o seu frontend espera:
        std::stringstream ss;
        ss << "ENDERECO (MZ):\n" << w.address << "\n\nSEED:\n>> " << w.seed << " <<";
        
        crow::response res(ss.str());
        add_cors(res);
        return res;
    });

    CROW_ROUTE(app, "/stats")([&bc]() {
        crow::json::wvalue x;
        x["height"] = (int)bc.getChain().size();
        x["difficulty"] = bc.getDifficulty();
        crow::response res(x);
        add_cors(res);
        return res;
    });

    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string endereco) {
        crow::json::wvalue x;
        x["address"] = endereco;
        x["balance"] = bc.getBalance(endereco);
        crow::response res{x};
        add_cors(res);
        return res;
    });

    CROW_ROUTE(app, "/minerar_agora/<string>")([&bc](std::string endereco) {
        crow::json::wvalue x;
        
        // Segurança Básica: Verifica se o endereço começa com o prefixo da sua moeda
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

    // Pega a porta do Render ou usa a 10000
    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 10000;
    
    std::cout << "MazeChain rodando na porta " << port << std::endl;
    app.port(port).multithreaded().run();
}
