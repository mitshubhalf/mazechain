#include "crow_all.h"
#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include <vector>
#include <string>
#include <cstdlib>
#include <set>
#include <curl/curl.h> // Para sincronizar com outros nós

// Lista de endereços de outros nós (Peers)
std::set<std::string> peers;

int main() {
    crow::SimpleApp app;
    Blockchain bc;
    
    // Carrega a chain existente ao iniciar o servidor
    Storage::loadChain(bc, "data/blockchain.dat");

    // ROTA RAIZ: Para evitar o erro 404 ao acessar a URL pura
    CROW_ROUTE(app, "/")([]() {
        return "<h1>MazeChain API Online</h1><p>Use /stats, /chain ou /wallet/new</p>";
    });

    // ROTA: Criar Nova Carteira (MOSTRANDO A SEED PARA TESTES)
    CROW_ROUTE(app, "/wallet/new")([]() {
        Wallet w;
        w.create(); 
        crow::json::wvalue x;
        x["address"] = w.address;
        x["seed"] = w.seed; // Agora você verá as 12 palavras no navegador
        x["info"] = "AVISO: Guarde sua seed! Ela e sua senha de acesso.";
        return x;
    });

    // ROTA: Registrar um novo nó na rede
    CROW_ROUTE(app, "/nodes/register").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !body.has("node_url")) return crow::response(400, "URL do nó ausente");
        peers.insert(body["node_url"].s());
        return crow::response(200, "No registrado com sucesso");
    });

    // ROTA: Listar nós conhecidos
    CROW_ROUTE(app, "/nodes/list")([]() {
        crow::json::wvalue x;
        std::vector<std::string> p_list(peers.begin(), peers.end());
        x["peers"] = p_list;
        return x;
    });

    // ROTA: Consenso (Sincronizar com a maior corrente da rede)
    CROW_ROUTE(app, "/nodes/resolve")([&bc]() {
        bool replaced = false;
        // A lógica de busca HTTP via CURL seria implementada aqui
        
        crow::json::wvalue x;
        x["message"] = replaced ? "Corrente substituida" : "Sua corrente e a maior";
        x["length"] = (int)bc.getChain().size();
        return x;
    });

    // ROTA: Estatísticas Gerais
    CROW_ROUTE(app, "/stats")([&bc]() {
        crow::json::wvalue x;
        x["height"] = (int)bc.getChain().size();
        x["difficulty"] = bc.getDifficulty();
        x["peers_count"] = (int)peers.size();
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
            blocks.push_back(block);
        }
        return crow::json::wvalue(blocks);
    });

    // ROTA: Minerar (POST)
    CROW_ROUTE(app, "/mine").methods(crow::HTTPMethod::POST)
    ([&bc](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !body.has("miner_address")) return crow::response(400, "Invalid JSON");
        
        bc.mineBlock(body["miner_address"].s());
        
        crow::json::wvalue x;
        x["status"] = "success";
        x["new_height"] = (int)bc.getChain().size();
        return crow::response(x);
    });

    // ROTA: Consultar Saldo
    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string address) {
        crow::json::wvalue x;
        x["address"] = address;
        x["balance"] = bc.getBalance(address);
        return x;
    });

    // --- AJUSTE PARA O RENDER ---
    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 10000;
    
    app.port(port).multithreaded().run();
}
