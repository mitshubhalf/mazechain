#define CROW_MAIN
#define CROW_ENABLE_CORS
#include "crow_all.h"
#include "blockchain.h"
#include "storage.h"
#include "wallet.h"
#include "transaction.h"
#include "p2p.h"
#include "node_manager.h"
#include "mining_utils.h"
#include <vector>
#include <string>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <memory>
#include <ctime>

// DEFINIÇÃO DOS CAMINHOS ABSOLUTOS PARA UNIFICAÇÃO
const std::string ABS_DB_PATH = "/home/runner/workspace/data/blockchain.dat";
const std::string ABS_MEMPOOL_PATH = "/home/runner/workspace/data/mempool.dat";

// Middleware de CORS aprimorado para aceitar requisições de outras portas (como a 5000)
struct CORS {
    struct context {};
    void before_handle(crow::request& req, crow::response& res, context&) {
        res.set_header("Access-Control-Allow-Origin", "*"); 
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Origin, Content-Type, Accept, Authorization");

        // Resposta para Pre-flight requests (necessário para o fetch do navegador)
        if (req.method == crow::HTTPMethod::OPTIONS) {
            res.code = 204;
            res.end();
        }
    }
    void after_handle(crow::request&, crow::response& res, context&) {}
};

int main() {
    // Verificação de dependência crítica
    std::ifstream check_wordlist("wordlist.txt");
    if (!check_wordlist.is_open()) {
        std::cerr << "CRITICAL ERROR: wordlist.txt missing! Wallet functions will fail." << std::endl;
        return 1;
    }
    check_wordlist.close();

    crow::App<CORS> app;

    // INICIALIZAÇÃO DO NÚCLEO CORE
    Blockchain bc; 
    P2P p2p;                                    
    NodeManager node_manager(bc, p2p);        

    // Cache do Index para servir via Root
    static std::string index_cache;
    std::ifstream file("index.html");
    if (file.is_open()) {
        index_cache.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
    }

    // Garante que a chain atualizada seja carregada no início usando o caminho absoluto
    Storage::loadChain(bc, ABS_DB_PATH);

    // Sincronização inicial com Peers conhecidos
    node_manager.sync_network();              

    // ROTA RAIZ - Serve o Frontend
    CROW_ROUTE(app, "/")([]() {
        if (!index_cache.empty()) {
            crow::response res(index_cache);
            res.set_header("Content-Type", "text/html");
            return res;
        }
        return crow::response(200, "MAZECHAIN NODE v2.1.1 - [BACKEND ONLINE]");
    });

    // STATUS GERAL DO NÓ (Usado pela aba Rede)
    CROW_ROUTE(app, "/status")([&bc, &p2p]() { 
        crow::json::wvalue x;
        x["network"]["status"] = "operational";
        x["network"]["peers_count"] = (int)p2p.peers.size(); 
        x["network"]["version"] = "2.1.1-PROD";
        x["protocol"]["difficulty"] = bc.getDifficulty();
        x["protocol"]["total_supply"] = bc.getTotalSupply();
        x["chain"]["height"] = (int)bc.getChain().size();
        x["timestamp"] = std::time(nullptr);
        return crow::response(x);
    });

    // CONSULTA DE SALDO (Usado pela aba Carteira)
    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string endereco) {
        double bal = bc.getBalance(endereco);
        crow::json::wvalue x;
        x["address"] = endereco;
        x["balance_mz"] = bal;
        x["balance_mits"] = (long long)(bal * 100000000.0);
        return crow::response(x);
    });

    // GERAÇÃO DE CARTEIRA
    CROW_ROUTE(app, "/wallet/new")([]() {
        Wallet w;
        w.create(); // Gera seed mnemônica e endereço MZ
        crow::json::wvalue result;
        result["address"] = w.address;
        result["seed"] = w.seed;
        return crow::response(result);
    });

    // IMPORTAÇÃO DE CARTEIRA (POST)
    CROW_ROUTE(app, "/wallet/import").methods(crow::HTTPMethod::POST)([](const crow::request& req) {
        auto x = crow::json::load(req.body);
        if (!x || !x.has("seed")) return crow::response(400, "Seed missing");

        Wallet w;
        try {
            // Chamando a função fromSeed conforme definido no seu wallet.h/cpp
            w.fromSeed(x["seed"].s()); 
            crow::json::wvalue result;
            result["address"] = w.address;
            result["status"] = "success";
            return crow::response(result);
        } catch (const std::exception& e) {
            return crow::response(400, "Invalid mnemonic phrase");
        } catch (...) {
            return crow::response(400, "Unknown error during import");
        }
    });

    // ENVIO DE TRANSAÇÃO (Usado pela aba Carteira)
    CROW_ROUTE(app, "/send").methods(crow::HTTPMethod::POST)([&bc, &p2p](const crow::request& req) {
        auto x = crow::json::load(req.body);
        crow::json::wvalue result;
        if (!x || !x.has("from") || !x.has("to") || !x.has("amount") || !x.has("seed")) {
            result["error"] = "Missing parameters";
            return crow::response(400, result);
        }
        try {
            bc.send(x["from"].s(), x["to"].s(), x["amount"].d(), x["seed"].s());

            // Notifica a rede P2P
            p2p.broadcast_transaction(req.body); 

            result["status"] = "success";
            return crow::response(200, result);
        } catch (const std::exception& e) {
            result["status"] = "error";
            result["reason"] = e.what();
            return crow::response(400, result);
        }
    });

    // EXPLORADOR DE BLOCOS (Usado pela aba Blockchain)
    CROW_ROUTE(app, "/chain")([&bc]() {
        crow::json::wvalue x;
        const auto& chain = bc.getChain();
        std::vector<crow::json::wvalue> block_list;

        int start = std::max(0, (int)chain.size() - 50);
        for (size_t i = start; i < chain.size(); ++i) {
            const auto& block = chain[i];
            crow::json::wvalue b;
            b["index"] = block.index;
            b["hash"] = block.hash;
            b["prev_hash"] = block.prevHash;
            b["tx_count"] = (int)block.transactions.size();
            b["timestamp"] = (long long)block.timestamp;
            block_list.push_back(std::move(b));
        }
        x["blocks"] = std::move(block_list);
        x["tip_height"] = (int)chain.size();
        return crow::response(x);
    });

    // MINERAÇÃO MANUAL VIA API
    CROW_ROUTE(app, "/minerar_agora/<string>")([&bc, &p2p](std::string endereco) {
        if (endereco.size() < 10) return crow::response(400, "Invalid Address");
        try {
            int prev_height = (int)bc.getChain().size();

            bc.mineBlock(endereco);

            // Salvamento e limpeza usando os caminhos absolutos definidos
            Storage::saveChain(bc, ABS_DB_PATH);
            Storage::clearMempool(ABS_MEMPOOL_PATH);

            p2p.broadcast_block("NEW_BLOCK_MINED"); 

            crow::json::wvalue x;
            x["status"] = "success";
            x["new_height"] = (int)bc.getChain().size();
            x["reward"] = bc.getBlockReward(prev_height);
            return crow::response(x);
        } catch (const std::exception& e) {
            return crow::response(500, std::string("Mining Error: ") + e.what());
        }
    });

    // GESTÃO DE PEERS (P2P)
    CROW_ROUTE(app, "/p2p/add_node/<string>")([&p2p](std::string node_url) {
        p2p.add_peer(node_url);
        return crow::response(200, "Peer added");
    });

    // CONFIGURAÇÃO DA PORTA E EXECUÇÃO
    const char* port_env = std::getenv("PORT");
    int port = (port_env != nullptr) ? std::stoi(port_env) : 10000;

    std::cout << "========================================" << std::endl;
    std::cout << "  MAZECHAIN NODE v2.1.1 ONLINE" << std::endl;
    std::cout << "  Porta API (Backend): " << port << std::endl;
    std::cout << "  Database: " << ABS_DB_PATH << std::endl;
    std::cout << "========================================" << std::endl;

    app.port(port).bindaddr("0.0.0.0").multithreaded().run();
}