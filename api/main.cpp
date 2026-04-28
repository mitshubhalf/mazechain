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
#include "crypto.h"
#include "httplib.h"
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
#include <thread>
#include <atomic> 
#include <csignal> 
#include <sys/stat.h>

#ifndef _WIN32
#include <termios.h>
#include <unistd.h>
#define MKDIR(path) mkdir(path, 0777)
#else
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#endif

// --- VARIÁVEIS DE CONTROLE GLOBAL ---
std::atomic<bool> is_mining{false};          
std::atomic<bool> global_keep_running{true}; 

const std::string ABS_DATA_PATH = "data";
const std::string ABS_DB_PATH = "data/blockchain.dat";
const std::string ABS_MEMPOOL_PATH = "data/mempool.dat";
const std::string ABS_WALLET_PATH = "data/wallet.dat";

void signal_handler(int signum) {
    std::cout << "\n[SISTEMA] Sinal (" << signum << ") recebido. Desligando..." << std::endl;
    global_keep_running = false;
    is_mining = false;
}

// --- FUNÇÃO DE MINERAÇÃO EM THREAD (MELHORIA) ---
void async_mine(Blockchain& bc, P2P& p2p, std::string addr) {
    if (is_mining) return;
    is_mining = true;

    try {
        std::cout << "\n[MOTOR] ⛏️ Minerando Bloco #" << bc.getChain().size() << " via CSHA256..." << std::endl;
        bc.mineBlock(addr); 

        Storage::saveChain(bc, ABS_DB_PATH);
        Storage::clearMempool(ABS_MEMPOOL_PATH);
        p2p.broadcast_block("{\"status\": \"new_block_mined\"}");

        std::cout << "[MOTOR] ✅ Bloco finalizado e salvo." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[ERRO] Falha na mineração: " << e.what() << std::endl;
    }

    is_mining = false;
}

std::string get_hidden_password(const std::string& prompt) {
    std::cout << prompt;
    std::string password;
#ifdef _WIN32
    std::cin >> password;
#else
    termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    std::getline(std::cin, password);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    std::cout << std::endl;
#endif
    return password;
}

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
    void after_handle(crow::request&, crow::response&, context&) {}
};

void print_help() {
    std::cout << "\n==========================================\n";
    std::cout << "      MAZECHAIN CORE v3.0.0 (DYNAMIC)     \n";
    std::cout << "==========================================\n";
    std::cout << "Uso: ./mazechain [comando]\n\n";
    std::cout << "Comandos:\n";
    std::cout << "  mine [endereco]                - Inicia minerador\n";
    std::cout << "  balance [endereco]             - Consulta saldo\n";
    std::cout << "  wallet create                  - Nova carteira (apenas seed)\n";
    std::cout << "  wallet create-secure           - Nova carteira criptografada no disco\n";
    std::cout << "  wallet from-seed \"<seed>\"     - Recupera endereço\n";
    std::cout << "  chain                          - Lista blocos\n";
    std::cout << "  chain stats                    - Supply e estatísticas\n";
    std::cout << "  chain validate                 - Valida integridade\n";
    std::cout << "  mempool                        - Lista pendentes\n";
    std::cout << "  send [para] [qtd]              - Envia MZ usando wallet.dat\n";
    std::cout << "  send [de] [para] [qtd] [seed] - Envia MZ (Modo legado)\n";
    std::cout << "  p2p add [url]                  - Adiciona um peer\n";
    std::cout << "==========================================\n\n";
}

void run_httplib_server() {
    httplib::Server svr;
    svr.Post("/block", [](const httplib::Request&, httplib::Response& res) {
        std::cout << "\n📡 [REDE] Novo bloco recebido via httplib..." << std::endl;
        res.set_content("Recebido", "text/plain");
    });
    svr.Post("/tx", [](const httplib::Request&, httplib::Response& res) {
        std::cout << "\n📡 [REDE] Nova transação recebida via httplib..." << std::endl;
        res.set_content("Recebida", "text/plain");
    });
    svr.listen("0.0.0.0", 8080);
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    MKDIR(ABS_DATA_PATH.c_str());

    auto bc_ptr = std::make_unique<Blockchain>();
    auto p2p_ptr = std::make_unique<P2P>();
    Blockchain& bc = *bc_ptr;
    P2P& p2p = *p2p_ptr;

    std::cout << "📦 Carregando dados locais..." << std::endl;
    Storage::loadChain(bc, ABS_DB_PATH);
    NodeManager node_manager(bc, p2p);

    std::thread httplib_thread(run_httplib_server);
    httplib_thread.detach();

    if (argc > 1) {
        std::string cmd = argv[1];
        if (cmd == "mine" && argc > 2) {
            async_mine(bc, p2p, argv[2]);
            return 0;
        }
        if (cmd == "balance" && argc > 2) {
            double bal = bc.getBalance(argv[2]);
            std::cout << "\n💰 Endereço: " << argv[2] << "\nSalto: " << std::fixed << std::setprecision(8) << bal << " MZ\n";
            return 0;
        }
        if (cmd == "wallet" && argc > 2) {
            std::string sub = argv[2];
            if (sub == "create") {
                Wallet w; w.create();
                std::cout << "✅ ADDR: " << w.address << "\nSEED: " << w.seed << "\n";
            } else if (sub == "from-seed" && argc > 3) {
                Wallet w; w.fromSeed(argv[3]);
                std::cout << "✅ ADDR: " << w.address << "\n";
            } else if (sub == "create-secure") {
                std::string seed, pass;
                std::cout << "SEED: "; std::getline(std::cin, seed);
                pass = get_hidden_password("SENHA: ");
                Wallet w; w.fromSeed(seed);
                std::string encrypted = Crypto::encrypt_data(seed, pass);
                Storage::saveWallet(w.address, encrypted, ABS_WALLET_PATH);
                std::cout << "✅ Carteira salva em " << ABS_WALLET_PATH << "\n";
            }
            return 0;
        }
        if (cmd == "chain") {
            std::string sub = (argc > 2) ? argv[2] : "";
            if (sub == "stats") bc.printStats();
            else if (sub == "validate") {
                if (bc.isChainValid()) std::cout << "✅ Blockchain íntegra.\n";
                else std::cout << "❌ Falha na integridade!\n";
            } else {
                for (const auto& b : bc.getChain()) std::cout << "Bloco #" << b.index << " | Hash: " << b.hash.substr(0,16) << "...\n";
            }
            return 0;
        }
        if (cmd == "send") {
            try {
                if (argc == 4) {
                    std::string addr, encKey;
                    Storage::loadWallet(addr, encKey, ABS_WALLET_PATH);
                    std::string decSeed = Crypto::decrypt_data(encKey, get_hidden_password("SENHA: "));
                    bc.send(addr, argv[2], std::stod(argv[3]), decSeed);
                } else if (argc >= 6) {
                    bc.send(argv[2], argv[3], std::stod(argv[4]), argv[5]);
                }
                p2p.broadcast_transaction("{\"info\": \"new_tx\"}");
                std::cout << "✅ Transação enviada.\n";
            } catch (const std::exception& e) { std::cerr << "❌ Erro: " << e.what() << "\n"; }
            return 0;
        }
        if (cmd == "mempool") {
            auto pending = Storage::loadMempool(ABS_MEMPOOL_PATH);
            for (const auto& tx : pending) std::cout << "ID: " << tx.id.substr(0,10) << " | " << tx.vout[0].amount << " MZ\n";
            return 0;
        }
        if (cmd == "p2p" && argc >= 4 && std::string(argv[2]) == "add") {
            p2p.add_peer(argv[3]);
            return 0;
        }
        if (cmd == "help") { print_help(); return 0; }
    }

    crow::App<CORS> app;

    CROW_ROUTE(app, "/status")([&bc, &p2p](){
        crow::json::wvalue x;
        x["peers"] = (int)p2p.peers.size();
        x["height"] = (int)bc.getChain().size();
        x["supply"] = bc.getTotalSupply();
        x["difficulty"] = bc.getDifficulty();
        x["mining_status"] = is_mining.load() ? "active" : "idle";
        return x;
    });

    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string addr){
        crow::json::wvalue x;
        x["balance"] = bc.getBalance(addr);
        return x;
    });

    CROW_ROUTE(app, "/send").methods(crow::HTTPMethod::POST)([&bc, &p2p](const crow::request& req){
        auto x = crow::json::load(req.body);
        if (!x) return crow::response(400, "JSON Inválido");
        try {
            bc.send(x["from"].s(), x["to"].s(), x["amount"].d(), x["seed"].s());
            p2p.broadcast_transaction(req.body);
            return crow::response(200, "Transação Enviada");
        } catch (const std::exception& e) { return crow::response(400, e.what()); }
    });

    CROW_ROUTE(app, "/minerar_agora/<string>")([&bc, &p2p](std::string addr){
        if (is_mining) return crow::response(429, "Minerador Ocupado");

        // DISPARA MINERAÇÃO EM THREAD PARA NÃO TRAVAR A API
        std::thread t(async_mine, std::ref(bc), std::ref(p2p), addr);
        t.detach(); 

        return crow::response(200, "Mineração Iniciada em Background");
    });

    CROW_ROUTE(app, "/chain")([&bc](){
        crow::json::wvalue x;
        auto chain = bc.getChain();
        for(size_t i=0; i<chain.size(); i++) {
            x[i]["index"] = (int)chain[i].index;
            x[i]["hash"] = chain[i].hash;
            x[i]["prev_hash"] = chain[i].prevHash;
            x[i]["timestamp"] = (int)chain[i].timestamp;
        }
        return x;
    });

    int port = std::getenv("PORT") ? std::stoi(std::getenv("PORT")) : 10000;
    std::cout << "\n🚀 MAZECHAIN NÓ INICIADO | PORTA: " << port << std::endl;

    app.port(port).multithreaded().run();
}