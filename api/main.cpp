#include "crow_all.h"
#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include <vector>
#include <string>
#include <cstdlib>
#include <sstream>
#include <iostream>

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

    CROW_ROUTE(app, "/wallet/new")([]() {
        Wallet w;
        w.create();
        std::stringstream ss;
        // Mantém o formato esperado pelo Regex do Frontend
        ss << "ENDERECO (MZ):\n" << w.address << "\n\nSEED:\n>> " << w.seed << " <<";
        crow::response res(ss.str());
        add_cors(res);
        return res;
    });

    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string endereco) {
        double saldo = bc.getBalance(endereco);
        
        // BLOQUEIO DE SEGURANÇA:
        // Se o endereço não tem saldo (0) e a chain não o reconhece, 
        // retorna 404 para impedir login de seeds falsas no frontend.
        if (saldo <= 0) {
            crow::response res(404, "Carteira nao registrada ou sem saldo.");
            add_cors(res);
            return res;
        }

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
        // Salva a blockchain no disco imediatamente
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
