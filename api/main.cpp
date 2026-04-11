#include "crow_all.h"
#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include <vector>
#include <string>

int main() {
    crow::SimpleApp app;
    Blockchain bc;
    
    // Carrega a chain existente ao iniciar o servidor
    Storage::loadChain(bc, "data/blockchain.dat");

    // ROTA: Estatísticas Gerais
    CROW_ROUTE(app, "/stats")([&bc]() {
        crow::json::wvalue x;
        x["height"] = (int)bc.getChain().size();
        x["difficulty"] = bc.getDifficulty();
        return x;
    });

    // ROTA: Visualizar a Chain completa (JSON)
    CROW_ROUTE(app, "/chain")([&bc]() {
        std::vector<crow::json::wvalue> blocks;
        for (const auto& b : bc.getChain()) {
            crow::json::wvalue block;
            block["index"] = b.index;
            block["hash"] = b.hash;
            block["prevHash"] = b.prevHash;
            block["timestamp"] = (long long)b.timestamp;
            block["tx_count"] = (int)b.transactions.size();
            blocks.push_back(block);
        }
        return crow::json::wvalue(blocks);
    });

    // ROTA: Consultar Saldo
    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string address) {
        crow::json::wvalue x;
        x["address"] = address;
        x["balance"] = bc.getBalance(address);
        return x;
    });

    // ROTA: Criar Nova Carteira (via API)
    CROW_ROUTE(app, "/wallet/new")([]() {
        Wallet w;
        w.create(); // Isso imprime no console, mas para a API vamos precisar capturar
        crow::json::wvalue x;
        x["address"] = w.address;
        x["info"] = "Check server console for mnemonic seed (security reasons)";
        return x;
    });

    // ROTA: Minerar (POST)
    CROW_ROUTE(app, "/mine").methods(crow::HTTPMethod::POST)
    ([&bc](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !body.has("miner_address")) return crow::response(400, "Invalid JSON");
        
        std::string miner = body["miner_address"].s();
        bc.mineBlock(miner);
        
        crow::json::wvalue x;
        x["status"] = "success";
        x["new_height"] = (int)bc.getChain().size();
        return crow::response(x);
    });

    // ROTA: Enviar Transação (POST)
    CROW_ROUTE(app, "/send").methods(crow::HTTPMethod::POST)
    ([&bc](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "Invalid JSON");

        std::string from = body["from"].s();
        std::string to = body["to"].s();
        double amount = body["amount"].d();
        std::string seed = body["seed"].s();

        // IMPORTANTE: Aqui precisamos simular o comportamento do 'send' 
        // mas sem o 'std::cin' para não travar o servidor.
        bc.send(from, to, amount); // Note: ajuste sua bc.send para aceitar a seed como param opcional se quiser automatizar 100%
        
        return crow::response(200, "Transaction pushed to mempool");
    });

    app.port(8080).multithreaded().run();
}
