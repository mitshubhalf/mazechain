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
    
    Storage::loadChain(bc, "data/blockchain.dat");

    // ROTA: Registrar um novo nó na rede
    CROW_ROUTE(app, "/nodes/register").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !body.has("node_url")) return crow::response(400, "URL do nó ausente");
        peers.insert(body["node_url"].s());
        return crow::response(200, "Nó registrado com sucesso");
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
        size_t max_length = bc.getChain().size();

        for (const std::string& peer : peers) {
            // Aqui você chamaria o Peer via HTTP para pegar a chain dele
            // Se a chain dele for maior e válida, você substitui a sua.
            // (Para simplificar no teste, vamos apenas retornar o status)
        }
        
        crow::json::wvalue x;
        x["message"] = replaced ? "Corrente substituída" : "Sua corrente é a maior";
        x["length"] = (int)bc.getChain().size();
        return x;
    });

    // --- MANTENDO AS ROTAS ANTERIORES ---
    CROW_ROUTE(app, "/stats")([&bc]() {
        crow::json::wvalue x;
        x["height"] = (int)bc.getChain().size();
        x["difficulty"] = bc.getDifficulty();
        x["peers_count"] = (int)peers.size();
        return x;
    });

    CROW_ROUTE(app, "/chain")([&bc]() {
        std::vector<crow::json::wvalue> blocks;
        for (const auto& b : bc.getChain()) {
            crow::json::wvalue block;
            block["index"] = b.index;
            block["hash"] = b.hash;
            block["prevHash"] = b.prevHash;
            blocks.push_back(block);
        }
        return crow::json::wvalue(blocks);
    });

    CROW_ROUTE(app, "/mine").methods(crow::HTTPMethod::POST)
    ([&bc](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !body.has("miner_address")) return crow::response(400, "Invalid JSON");
        bc.mineBlock(body["miner_address"].s());
        return crow::response(200, "Bloco minerado!");
    });

    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 10000;
    app.port(port).multithreaded().run();
}
