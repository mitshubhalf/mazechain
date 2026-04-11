#include "crow_all.h"
#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include <vector>
#include <string>
#include <cstdlib>
#include <set>
#include <sstream>
#include <curl/curl.h>

// Lista de endereços de outros nós (Peers)
std::set<std::string> peers;

int main() {
    crow::SimpleApp app;
    Blockchain bc;
    
    // Carrega a chain existente ao iniciar o servidor
    Storage::loadChain(bc, "data/blockchain.dat");

    // ROTA RAIZ: Interface simples para facilitar no celular
    CROW_ROUTE(app, "/")([]() {
        std::stringstream html;
        html << "<html><body style='font-family:sans-serif; padding:20px;'>";
        html << "<h1>MazeChain Node Online</h1>";
        html << "<ul>";
        html << "<li><a href='/wallet/new'><b>CRIAR NOVA CARTEIRA (Ver Seed)</b></a></li>";
        html << "<li><a href='/stats'>Ver Status</a></li>";
        html << "<li><a href='/chain'>Ver Blockchain</a></li>";
        html << "</ul>";
        html << "</body></html>";
        return crow::response(html.str());
    });

    // ROTA: Criar Nova Carteira (Melhorado para exibição no celular)
    CROW_ROUTE(app, "/wallet/new")([]() {
        Wallet w;
        w.create(); 
        
        std::stringstream ss;
        ss << "====================================\n";
        ss << "      NOVA CARTEIRA MAZECHAIN       \n";
        ss << "====================================\n\n";
        ss << "ENDERECO (MZ):\n" << w.address << "\n\n";
        ss << "SUA SEED (12 PALAVRAS):\n";
        ss << ">> " << w.seed << " <<\n\n";
        ss << "====================================\n";
        ss << "AVISO: ANOTE AS PALAVRAS ACIMA AGORA!\n";
        ss << "SE PERDER A SEED, PERDE AS MOEDAS.\n";
        ss << "====================================";
        
        return crow::response(ss.str());
    });

    // ROTA: Registrar um novo nó na rede
    CROW_ROUTE(app, "/nodes/register").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !body.has("node_url")) return crow::response(400, "URL do no ausente");
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

    // ROTA: Consenso (Resolve conflitos de rede)
    CROW_ROUTE(app, "/nodes/resolve")([&bc]() {
        bool replaced = false;
        size_t current_len = bc.getChain().size();
        
        // Aqui o nó percorreria a lista 'peers' pedindo a /chain de cada um
        // usando a biblioteca CURL instalada no seu Docker.
        
        crow::json::wvalue x;
        x["message"] = replaced ? "Corrente substituida" : "Sua corrente e a maior ou igual";
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

    // ROTA: Visualizar a Chain completa
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
        if (!body || !body.has("miner_address")) return crow::response(400, "Address necessario");
        
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
