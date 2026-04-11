#include "crow_all.h"
#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include <vector>
#include <string>
#include <cstdlib> // Necessário para std::getenv

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
        w.create(); 
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
        
        bc.send(from, to, amount); 
        return crow::response(200, "Transaction pushed to mempool");
    });

    // --- AJUSTE PARA O RENDER ---
    // Ele tenta ler a porta que o Render mandar. Se não tiver nenhuma, usa a 10000.
    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 10000;
    
    std::cout << "Servidor iniciando na porta " << port << std::endl;
    app.port(port).multithreaded().run();
}
