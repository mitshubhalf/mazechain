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
#include "httplib.h" // Inclusão do httplib do segundo código
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
#include <sys/stat.h>

#ifndef _WIN32
#include <termios.h>
#include <unistd.h>
#define MKDIR(path) mkdir(path, 0777)
#else
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#endif

// --- DEFINIÇÃO DOS CAMINHOS ---
const std::string ABS_DATA_PATH = "data";
const std::string ABS_DB_PATH = "data/blockchain.dat";
const std::string ABS_MEMPOOL_PATH = "data/mempool.dat";
const std::string ABS_WALLET_PATH = "data/wallet.dat";

// --- FUNÇÕES AUXILIARES ---
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
    std::cout << "  mine [endereco]               - Inicia minerador\n";
    std::cout << "  balance [endereco]            - Consulta saldo\n";
    std::cout << "  wallet create                 - Nova carteira (apenas seed)\n";
    std::cout << "  wallet create-secure          - Nova carteira criptografada no disco\n";
    std::cout << "  wallet from-seed \"<seed>\"     - Recupera endereço\n";
    std::cout << "  chain                         - Lista blocos\n";
    std::cout << "  chain stats                   - Supply e estatísticas\n";
    std::cout << "  chain validate                - Valida integridade\n";
    std::cout << "  mempool                       - Lista pendentes\n";
    std::cout << "  send [para] [qtd]             - Envia MZ usando wallet.dat\n";
    std::cout << "  send [de] [para] [qtd] [seed] - Envia MZ (Modo legado)\n";
    std::cout << "  p2p add [url]                 - Adiciona um peer\n";
    std::cout << "==========================================\n\n";
}

// --- SERVIDOR P2P HTTPLIB (PARA COMPATIBILIDADE) ---
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
    // 1. Garante pastas e wordlist
    MKDIR(ABS_DATA_PATH.c_str());
    std::ifstream check_wordlist("wordlist.txt");
    if (!check_wordlist.is_open()) {
        std::ofstream create_wl("wordlist.txt");
        create_wl << "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";
        create_wl.close();
    } else { check_wordlist.close(); }

    // 2. Inicialização Core
    auto bc_ptr = std::make_unique<Blockchain>();
    auto p2p_ptr = std::make_unique<P2P>();
    Blockchain& bc = *bc_ptr;
    P2P& p2p = *p2p_ptr;

    // 3. Carregamento do Disco
    std::cout << "📦 Carregando dados locais..." << std::endl;
    Storage::loadChain(bc, ABS_DB_PATH);
    NodeManager node_manager(bc, p2p);

    // 4. Inicia Servidor P2P secundário (httplib) em thread separada
    std::thread httplib_thread(run_httplib_server);
    httplib_thread.detach();

    // 5. LÓGICA DE COMANDOS CLI (Tratando o argc/argv do segundo código)
    if (argc > 1) {
        std::string cmd = argv[1];

        if (cmd == "mine" && argc > 2) {
            std::string minerAddr = argv[2];
            std::cout << "⛏️ Minerando bloco #" << bc.getChain().size() << "...\n";
            bc.mineBlock(minerAddr);
            Storage::saveChain(bc, ABS_DB_PATH);
            Storage::clearMempool(ABS_MEMPOOL_PATH);
            p2p.broadcast_block("{\"status\": \"new_block_mined\"}");
            std::cout << "✅ Bloco minerado e propagado.\n";
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
                std::string seed = argv[3];
                Wallet w; w.fromSeed(seed);
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
                if (argc == 4) { // Modo novo
                    std::string addr, encKey;
                    Storage::loadWallet(addr, encKey, ABS_WALLET_PATH);
                    std::string pass = get_hidden_password("SENHA: ");
                    std::string decSeed = Crypto::decrypt_data(encKey, pass);
                    bc.send(addr, argv[2], std::stod(argv[3]), decSeed);
                } else if (argc >= 6) { // Modo legado
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
            std::cout << "✅ Peer adicionado.\n";
            return 0;
        }
    }

    // 6. MODO NÓ COMPLETO (CROW API)
    crow::App<CORS> app;

    // Cache de index.html
    static std::string index_cache;
    std::ifstream file("index.html");
    if (file.is_open()) {
        index_cache.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    // Rotas do Crow (Unificado)
    CROW_ROUTE(app, "/")([](){ return index_cache.empty() ? "MAZECHAIN ONLINE" : index_cache; });

    CROW_ROUTE(app, "/status")([&bc, &p2p](){
        crow::json::wvalue x;
        x["peers"] = (int)p2p.peers.size();
        x["height"] = (int)bc.getChain().size();
        x["supply"] = bc.getTotalSupply();
        return x;
    });

    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string addr){
        crow::json::wvalue x;
        x["balance"] = bc.getBalance(addr);
        return x;
    });

    CROW_ROUTE(app, "/send").methods(crow::HTTPMethod::POST)([&bc, &p2p](const crow::request& req){
        auto x = crow::json::load(req.body);
        bc.send(x["from"].s(), x["to"].s(), x["amount"].d(), x["seed"].s());
        p2p.broadcast_transaction(req.body);
        return crow::response(200, "OK");
    });

    CROW_ROUTE(app, "/minerar_agora/<string>")([&bc, &p2p](std::string addr){
        bc.mineBlock(addr);
        Storage::saveChain(bc, ABS_DB_PATH);
        p2p.broadcast_block("NEW_BLOCK");
        return crow::response(200, "Mined");
    });

    CROW_ROUTE(app, "/p2p/add_node/<string>")([&p2p](std::string node){
        p2p.add_peer(node);
        return crow::response(200, "Added");
    });

    int port = std::getenv("PORT") ? std::stoi(std::getenv("PORT")) : 10000;
    std::cout << "🚀 Servidor MazeChain na porta " << port << std::endl;
    app.port(port).multithreaded().run();
}