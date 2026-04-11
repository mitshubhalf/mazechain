#include "crow_all.h"
#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include <vector>
#include <string>
#include <cstdlib>
#include <set>
#include <sstream>

// Lista de endereços de outros nós (Peers)
std::set<std::string> peers;

int main() {
    crow::SimpleApp app;
    Blockchain bc;
    
    // Carrega a chain existente ao iniciar o servidor
    Storage::loadChain(bc, "data/blockchain.dat");

    // ============================================================
    // ROTA RAIZ: Interface simples para facilitar no celular
    // ============================================================
    CROW_ROUTE(app, "/")([]() {
        std::stringstream html;
        html << "<html><body style='font-family:sans-serif; padding:20px; text-align:center;'>";
        html << "<h1>MazeChain Node Online</h1>";
        html << "<hr>";
        html << "<div style='display: inline-block; text-align: left;'>";
        html << "<h3>Menu:</h3>";
        html << "<ul>";
        html << "<li><a href='/wallet/new' style='font-size:1.2em;'><b>[ Gerar Nova Carteira ]</b></a></li>";
        html << "<li><a href='/stats'>Estatisticas do No</a></li>";
        html << "<li><a href='/chain'>Explorar Blocos (JSON)</a></li>";
        html << "</ul>";
        html << "</div>";
        html << "</body></html>";
        return crow::response(html.str());
    });

    // ============================================================
    // ROTA: Criar Nova Carteira (Gera e exibe a SEED corretamente)
    // ============================================================
    CROW_ROUTE(app, "/wallet/new")([]() {
        Wallet w;
        w.create(); // A lógica dentro de wallet.cpp agora preenche w.seed
        
        std::stringstream ss;
        ss << "====================================\n";
        ss << "      NOVA CARTEIRA MAZECHAIN       \n";
        ss << "====================================\n\n";
        ss << "ENDERECO (MZ):\n" << w.address << "\n\n";
        ss << "SUA SEED (12 PALAVRAS):\n";
        ss << ">> " << w.seed << " <<\n\n"; // Agora w.seed será exibida aqui
        ss << "====================================\n";
        ss << "AVISO: ANOTE AS PALAVRAS ACIMA AGORA!\n";
        ss << "SE PERDER A SEED, PERDE AS MOEDAS.\n";
        ss << "====================================";
        
        return crow::response(ss.str());
    });

    // ============================================================
    // ROTAS DE REDE (PEERS)
    // ============================================================
    CROW_ROUTE(app, "/nodes/register").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !body.has("node_url")) return crow::response(400, "URL do no ausente");
        peers.insert(body["node_url"].s());
        return crow::response(200, "No registrado com sucesso");
    });

    CROW_ROUTE(app, "/nodes/list")([]() {
        crow::json::wvalue x;
        std::vector<std::string> p_list(peers.begin(), peers.end());
        x["peers"] = p_list;
        return x;
    });

    // ============================================================
    // ROTAS DA BLOCKCHAIN
    // ============================================================
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
            block["timestamp"] = (long long)b.timestamp;
            blocks.push_back(block);
        }
        return crow::json::wvalue(blocks);
    });

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

    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string address) {
        crow::json::wvalue x;
        x["address"] = address;
        x["balance"] = bc.getBalance(address);
        return x;
    });

    // ============================================================
    // CONFIGURACAO DE PORTA (RENDER)
    // ============================================================
    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 10000;
    
    std::cout << "Servidor MazeChain rodando na porta " << port << "..." << std::endl;
    
    app.port(port).multithreaded().run();
}
