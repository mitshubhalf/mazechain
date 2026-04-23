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
#include <iomanip>

// DEFINIÇÃO DOS CAMINHOS ABSOLUTOS (Mantido conforme seu ambiente)
const std::string ABS_DB_PATH = "/home/runner/workspace/data/blockchain.dat";
const std::string ABS_MEMPOOL_PATH = "/home/runner/workspace/data/mempool.dat";

struct CORS {
    struct context {};
    void before_handle(crow::request& req, crow::response& res, context&) {
        res.set_header("Access-Control-Allow-Origin", "*"); 
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Origin, Content-Type, Accept, Authorization");
        if (req.method == crow::HTTPMethod::OPTIONS) {
            res.code = 204;
            res.end();
        }
    }
    void after_handle(crow::request&, crow::response& res, context&) {}
};

void print_help() {
    std::cout << "\n==========================================\n";
    std::cout << "          MAZECHAIN CORE v3.0.0 (DYNAMIC)   \n";
    std::cout << "==========================================\n";
    std::cout << "Uso: ./mazechain [comando]\n\n";
    std::cout << "Comandos:\n";
    std::cout << "  mine [endereco]           - Inicia minerador\n";
    std::cout << "  balance [endereco]        - Consulta saldo\n";
    std::cout << "  wallet create              - Nova carteira\n";
    std::cout << "  chain                     - Lista blocos\n";
    std::cout << "  send [de] [para] [qtd] [seed] - Envia MZ\n";
    std::cout << "==========================================\n\n";
}

int main(int argc, char* argv[]) {
    // Verificação de segurança da wordlist
    std::ifstream check_wordlist("wordlist.txt");
    if (!check_wordlist.is_open()) {
        std::cerr << "CRITICAL ERROR: wordlist.txt missing! Cannot initialize HD Wallets." << std::endl;
        return 1;
    }
    check_wordlist.close();

    // Inicialização do Core da Blockchain
    Blockchain bc; 
    P2P p2p;                                        
    NodeManager node_manager(bc, p2p);        

    // Carrega a chain do disco (Sempre necessário para sincronizar estado inicial)
    Storage::loadChain(bc, ABS_DB_PATH);

    // ============================================================
    // --- LÓGICA DE COMANDOS VIA TERMINAL (CLI) ---
    // ============================================================
    if (argc > 1) {
        std::string cmd = argv[1];

        // MODO MINERADOR INFINITO VIA TERMINAL
        if (cmd == "mine" && argc > 2) {
            std::string minerAddr = argv[2];
            std::cout << "⛏️ MODO MINERADOR ATIVADO [RECOMPENSAS DINÂMICAS]" << std::endl;
            while (true) {
                try {
                    // mineBlock agora aplica internamente adjustDifficulty() e getBlockReward()
                    bc.mineBlock(minerAddr);
                    Storage::saveChain(bc, ABS_DB_PATH);
                    Storage::clearMempool(ABS_MEMPOOL_PATH);

                    int current_height = bc.getChain().size() - 1;
                    std::cout << "✅ Bloco #" << current_height << " minerado com sucesso!" << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Erro no processo de mineração: " << e.what() << std::endl;
                }
            }
            return 0; 
        }

        // CONSULTA DE SALDO VIA TERMINAL
        if (cmd == "balance" && argc > 2) {
            double bal = bc.getBalance(argv[2]);
            std::cout << "\n💰 Endereço: " << argv[2] << "\n";
            std::cout << "💰 Saldo:    " << std::fixed << std::setprecision(8) << bal << " MZ\n";
            std::cout << "💎 Mits:     " << (long long)(bal * 100000000.0) << " units\n\n";
            return 0;
        }

        // GERAÇÃO DE CARTEIRA VIA TERMINAL
        if (cmd == "wallet" && argc > 2 && std::string(argv[2]) == "create") {
            Wallet w;
            w.create();
            std::cout << "\n✅ CARTEIRA GERADA:\nADDR: " << w.address << "\nSEED: " << w.seed << "\n\n";
            return 0;
        }

        // LISTAGEM DE BLOCOS VIA TERMINAL
        if (cmd == "chain") {
            std::cout << "\n--- HISTÓRICO DE BLOCOS (MAZECHAIN) ---\n";
            for (const auto& b : bc.getChain()) {
                std::cout << "Bloco #" << b.index << " | Hash: " << b.hash.substr(0,16) 
                          << "... | Reward: " << bc.getBlockReward(b.index) << " MZ | Txs: " << b.transactions.size() << "\n";
            }
            return 0;
        }

        // ENVIO DE TRANSAÇÃO VIA TERMINAL
        if (cmd == "send" && argc > 5) {
            try {
                bc.send(argv[2], argv[3], std::stod(argv[4]), argv[5]);
                std::cout << "✅ Transação enviada ao mempool com sucesso!\n";
            } catch (const std::exception& e) {
                std::cerr << "❌ Erro ao enviar transação: " << e.what() << std::endl;
            }
            return 0;
        }

        if (cmd == "--help" || cmd == "-h") {
            print_help();
            return 0;
        }
    }

    // ============================================================
    // --- MODO NÓ COMPLETO COM API REST (Crow) ---
    // ============================================================
    crow::App<CORS> app;
    static std::string index_cache;

    // Carrega o frontend (index.html) para o cache se existir
    std::ifstream file("index.html");
    if (file.is_open()) {
        index_cache.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
    }

    // Sincroniza com outros nós da rede antes de subir a API
    node_manager.sync_network();              

    // ROTA HOME - Serve o dashboard ou mensagem simples
    CROW_ROUTE(app, "/")([]() {
        if (!index_cache.empty()) {
            crow::response res(index_cache);
            res.set_header("Content-Type", "text/html");
            return res;
        }
        return crow::response(200, "MAZECHAIN NODE v3.0.0 - [ESTADO: OPERACIONAL]");
    });

    // ROTA DE STATUS - Retorna dados vitais da rede
    CROW_ROUTE(app, "/status")([&bc, &p2p]() { 
        crow::json::wvalue x;
        x["network"]["status"] = "operational";
        x["network"]["peers_count"] = (int)p2p.peers.size(); 
        x["network"]["version"] = "3.0.0-DYNAMIC-EMISSION";
        x["protocol"]["difficulty"] = bc.getDifficulty();
        x["protocol"]["total_supply"] = bc.getTotalSupply();
        x["protocol"]["max_supply"] = bc.getMaxSupply();
        x["chain"]["height"] = (int)bc.getChain().size();
        x["timestamp"] = std::time(nullptr);
        return crow::response(x);
    });

    // ROTA DE SALDO (API)
    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string endereco) {
        double bal = bc.getBalance(endereco);
        crow::json::wvalue x;
        x["address"] = endereco;
        x["balance_mz"] = bal;
        x["balance_mits"] = (long long)(bal * 100000000.0);
        return crow::response(x);
    });

    // GERAÇÃO DE CARTEIRA (API)
    CROW_ROUTE(app, "/wallet/new")([]() {
        Wallet w; w.create();
        crow::json::wvalue result;
        result["address"] = w.address;
        result["seed"] = w.seed;
        return crow::response(result);
    });

    // IMPORTAÇÃO DE CARTEIRA (API)
    CROW_ROUTE(app, "/wallet/import").methods(crow::HTTPMethod::POST)([](const crow::request& req) {
        auto x = crow::json::load(req.body);
        if (!x || !x.has("seed")) return crow::response(400, "Parâmetro 'seed' ausente.");
        Wallet w;
        try {
            w.fromSeed(x["seed"].s()); 
            crow::json::wvalue result;
            result["address"] = w.address;
            result["status"] = "success";
            return crow::response(result);
        } catch (...) { return crow::response(400, "Seed mnemônica inválida."); }
    });

    // ENVIO DE TRANSAÇÃO (API)
    CROW_ROUTE(app, "/send").methods(crow::HTTPMethod::POST)([&bc, &p2p](const crow::request& req) {
        auto x = crow::json::load(req.body);
        crow::json::wvalue result;
        if (!x || !x.has("from") || !x.has("to") || !x.has("amount") || !x.has("seed")) {
            result["error"] = "Parâmetros obrigatórios ausentes.";
            return crow::response(400, result);
        }
        try {
            bc.send(x["from"].s(), x["to"].s(), x["amount"].d(), x["seed"].s());
            p2p.broadcast_transaction(req.body); 
            result["status"] = "success";
            result["message"] = "Transação propagada na rede.";
            return crow::response(200, result);
        } catch (const std::exception& e) {
            result["status"] = "error";
            result["reason"] = e.what();
            return crow::response(400, result);
        }
    });

    // LISTAGEM DE BLOCOS (API)
    CROW_ROUTE(app, "/chain")([&bc]() {
        crow::json::wvalue x;
        const auto& chain = bc.getChain();
        std::vector<crow::json::wvalue> block_list;

        // Retorna os últimos 50 blocos para não sobrecarregar a resposta JSON
        int start = std::max(0, (int)chain.size() - 50);
        for (size_t i = start; i < chain.size(); ++i) {
            const auto& block = chain[i];
            crow::json::wvalue b;
            b["index"] = block.index;
            b["hash"] = block.hash;
            b["prev_hash"] = block.prevHash;
            b["reward"] = bc.getBlockReward(block.index); // Exibe recompensa correta do bloco
            b["tx_count"] = (int)block.transactions.size();
            b["timestamp"] = (long long)block.timestamp;
            block_list.push_back(std::move(b));
        }
        x["blocks"] = std::move(block_list);
        x["tip_height"] = (int)chain.size();
        return crow::response(x);
    });

    // MINERAÇÃO SOB DEMANDA (VIA BOTÃO NO FRONTEND)
    CROW_ROUTE(app, "/minerar_agora/<string>")([&bc, &p2p](std::string endereco) {
        try {
            int prev_height = (int)bc.getChain().size();

            // Executa o trabalho de PoW respeitando a nova dificuldade dinâmica
            bc.mineBlock(endereco);

            // Salva o novo estado no disco
            Storage::saveChain(bc, ABS_DB_PATH);
            Storage::clearMempool(ABS_MEMPOOL_PATH);

            // Propaga o novo bloco para outros nós (P2P)
            p2p.broadcast_block("NEW_BLOCK_MINED"); 

            crow::json::wvalue x;
            x["status"] = "success";
            x["new_height"] = (int)bc.getChain().size();
            x["reward_earned"] = bc.getBlockReward(prev_height);
            x["difficulty_used"] = bc.getDifficulty();
            return crow::response(x);
        } catch (const std::exception& e) {
            return crow::response(500, std::string("Falha no Minerador Interno: ") + e.what());
        }
    });

    // GERENCIAMENTO P2P - ADICIONAR NÓS MANUALMENTE
    CROW_ROUTE(app, "/p2p/add_node/<string>")([&p2p](std::string node_url) {
        p2p.add_peer(node_url);
        return crow::response(200, "Novo peer adicionado à lista de confiança.");
    });

    // CONFIGURAÇÃO DE PORTA (Pega do ambiente ou usa 10000 como padrão)
    const char* port_env = std::getenv("PORT");
    int port = (port_env != nullptr) ? std::stoi(port_env) : 10000;

    std::cout << "========================================" << std::endl;
    std::cout << "  MAZECHAIN NODE v3.0.0 ONLINE" << std::endl;
    std::cout << "  Porta API: " << port << std::endl;
    std::cout << "  Database: " << ABS_DB_PATH << std::endl;
    std::cout << "  Reward Base: 400 MZ | Max Supply: 20M" << std::endl;
    std::cout << "========================================" << std::endl;

    // Inicia o loop da API
    app.port(port).bindaddr("0.0.0.0").concurrency(2).run();
}