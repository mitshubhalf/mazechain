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

int main() {
    crow::SimpleApp app;
    Blockchain bc;
    Storage::loadChain(bc, "data/blockchain.dat");

    CROW_ROUTE(app, "/")([]() {
        return "MAZECHAIN NODE v1.0 - STATUS: ONLINE";
    });

    CROW_ROUTE(app, "/wallet/new")([]() {
        Wallet w;
        w.create();
        std::stringstream ss;
        ss << "ENDERECO (MZ):\n" << w.address << "\n\nSEED:\n" << w.seed;
        crow::response res(ss.str());
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });

    CROW_ROUTE(app, "/stats")([&bc]() {
        crow::json::wvalue x;
        x["height"] = (int)bc.getChain().size();
        x["difficulty"] = bc.getDifficulty();
        crow::response res(x);
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });

    // NOVA ROTA: SALDO
    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string endereco) {
        crow::json::wvalue x;
        x["address"] = endereco;
        x["balance"] = bc.getBalance(endereco);
        crow::response res{x};
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });

    CROW_ROUTE(app, "/minerar_agora/<string>")([&bc](std::string endereco) {
        bc.mineBlock(endereco);
        Storage::saveChain(bc, "data/blockchain.dat");
        crow::json::wvalue x;
        x["status"] = "success";
        x["blocos"] = (int)bc.getChain().size();
        crow::response res(x);
        res.add_header("Access-Control-Allow-Origin", "*");
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
        // CORREÇÃO: Chaves { } para evitar erro de inicialização
        crow::response res{crow::json::wvalue(blocks_json)};
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });

    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 10000;
    app.port(port).multithreaded().run();
}
