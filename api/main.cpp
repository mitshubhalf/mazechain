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
#include "crypto.h" // Adicionado para acesso às novas funções de criptografia
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

#ifndef _WIN32
#include <termios.h>
#include <unistd.h>
#endif

// DEFINIÇÃO DOS CAMINHOS ABSOLUTOS
const std::string ABS_DB_PATH = "data/blockchain.dat";
const std::string ABS_MEMPOOL_PATH = "data/mempool.dat";
const std::string ABS_WALLET_PATH = "data/wallet.dat"; // Caminho para a carteira segura

// Helper para ler senha sem mostrar no terminal (Padrão Linux/Home)
std::string get_hidden_password(const std::string& prompt) {
    std::cout << prompt;
    std::string password;
#ifdef _WIN32
    std::cin >> password; // Fallback simples para Windows
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
    void after_handle(crow::request& death, crow::response& res, context&) {}
};

void print_help() {
    std::cout << "\n==========================================\n";
    std::cout << "        MAZECHAIN CORE v3.0.0 (DYNAMIC)   \n";
    std::cout << "==========================================\n";
    std::cout << "Uso: ./mazechain [comando]\n\n";
    std::cout << "Comandos:\n";
    std::cout << "  mine [endereco]             - Inicia minerador\n";
    std::cout << "  balance [endereco]          - Consulta saldo\n";
    std::cout << "  wallet create               - Nova carteira (apenas seed)\n";
    std::cout << "  wallet create-secure        - Nova carteira criptografada no disco\n";
    std::cout << "  chain                       - Lista blocos\n";
    std::cout << "  send [para] [qtd]           - Envia MZ usando wallet.dat\n";
    std::cout << "  send [de] [para] [qtd] [seed] - Envia MZ (Modo legado)\n";
    std::cout << "==========================================\n\n";
}

int main(int argc, char* argv[]) {
    // 1. Verificação de segurança da wordlist
    std::ifstream check_wordlist("wordlist.txt");
    if (!check_wordlist.is_open()) {
        std::cerr << "CRITICAL ERROR: wordlist.txt missing! Creating emergency fallback..." << std::endl;
        std::ofstream create_wl("wordlist.txt");
        create_wl << "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";
        create_wl.close();
    } else {
        check_wordlist.close();
    }

    // 2. Inicialização Dinâmica
    auto bc_ptr = std::make_unique<Blockchain>();
    auto p2p_ptr = std::make_unique<P2P>();

    Blockchain& bc = *bc_ptr;
    P2P& p2p = *p2p_ptr;

    // 3. CARREGAMENTO DO DISCO
    std::cout << "📦 Carregando dados locais..." << std::endl;
    try {
        Storage::loadChain(bc, ABS_DB_PATH);
        std::cout << "✅ Blockchain carregada. Altura atual: " << bc.getChain().size() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "❌ Erro ao carregar banco de dados: " << e.what() << std::endl;
    }

    NodeManager node_manager(bc, p2p);

    // ============================================================
    // --- LÓGICA DE COMANDOS VIA TERMINAL (CLI) ---
    // ============================================================
    if (argc > 1) {
        std::string cmd = argv[1];

        if ((cmd == "mine" || cmd == "--mine") && argc > 2) {
            std::string minerAddr = argv[2];
            std::cout << "⛏️ MODO MINERADOR ATIVADO [RECOMPENSAS DINÂMICAS]" << std::endl;
            while (true) {
                try {
                    bc.mineBlock(minerAddr);
                    Storage::saveChain(bc, ABS_DB_PATH);
                    Storage::clearMempool(ABS_MEMPOOL_PATH);

                    int current_height = (int)bc.getChain().size() - 1;
                    std::cout << "✅ Bloco #" << current_height << " minerado com sucesso!" << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                } catch (const std::exception& e) {
                    std::cerr << "Erro no processo de mineração: " << e.what() << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                }
            }
            return 0; 
        }

        if (cmd == "balance" && argc > 2) {
            double bal = bc.getBalance(argv[2]);
            std::cout << "\n💰 Endereço: " << argv[2] << "\n";
            std::cout << "💰 Saldo:    " << std::fixed << std::setprecision(8) << bal << " MZ\n";
            std::cout << "💎 Mits:     " << (long long)(bal * 100000000.0) << " units\n\n";
            return 0;
        }

        if (cmd == "wallet" && argc > 2) {
            std::string sub = argv[2];
            if (sub == "create") {
                Wallet w;
                w.create();
                std::cout << "\n✅ CARTEIRA GERADA:\nADDR: " << w.address << "\nSEED: " << w.seed << "\n\n";
            } 
            else if (sub == "create-secure") {
                std::string seed, pass;
                std::cout << "Digite sua SEED (12 palavras): ";
                std::getline(std::cin, seed);
                pass = get_hidden_password("Defina uma SENHA para proteger seu arquivo: ");

                Wallet w;
                w.fromSeed(seed);
                std::string encrypted = Crypto::encrypt_data(seed, pass);
                Storage::saveWallet(w.address, encrypted, ABS_WALLET_PATH);

                std::cout << "✅ Carteira salva com segurança em " << ABS_WALLET_PATH << "!\n";
                std::cout << "Endereço: " << w.address << "\n";
            }
            return 0;
        }

        if (cmd == "chain") {
            std::cout << "\n--- HISTÓRICO DE BLOCOS (MAZECHAIN) ---\n";
            for (const auto& b : bc.getChain()) {
                std::cout << "Bloco #" << b.index << " | Hash: " << b.hash.substr(0,16) 
                          << "... | Reward: " << bc.getBlockReward(b.index) << " MZ | Txs: " << b.transactions.size() << "\n";
            }
            return 0;
        }

        if (cmd == "send") {
            try {
                // Modo Novo: ./mazechain send [para] [qtd]
                if (argc == 4) {
                    std::string addr, encKey;
                    if (!Storage::loadWallet(addr, encKey, ABS_WALLET_PATH)) {
                        throw std::runtime_error("Arquivo wallet.dat não encontrado. Use 'wallet create-secure' primeiro.");
                    }
                    std::string pass = get_hidden_password("Digite a SENHA da sua carteira: ");
                    std::string decryptedSeed = Crypto::decrypt_data(encKey, pass);

                    if (decryptedSeed == "DECRYPT_FAIL" || decryptedSeed == "ERROR") {
                        throw std::runtime_error("Senha incorreta! Não foi possível descriptografar a chave.");
                    }

                    bc.send(addr, argv[2], std::stod(argv[3]), decryptedSeed);
                    std::cout << "✅ Transação de " << argv[3] << " MZ enviada com sucesso!\n";
                }
                // Modo Legado: ./mazechain send [de] [para] [qtd] [seed]
                else if (argc >= 6) {
                    bc.send(argv[2], argv[3], std::stod(argv[4]), argv[5]);
                    std::cout << "✅ Transação enviada ao mempool com sucesso!\n";
                } else {
                    std::cout << "Uso correto:\n  ./mazechain send [para] [qtd]\n  ./mazechain send [de] [para] [qtd] [seed]\n";
                }
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

    std::ifstream file("index.html");
    if (file.is_open()) {
        index_cache.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
    }

    std::cout << "🌐 Sincronizando com peers da rede..." << std::endl;
    try {
        node_manager.sync_network();
    } catch (...) {
        std::cout << "⚠️ Aviso: Falha na sincronização inicial." << std::endl;
    }

    CROW_ROUTE(app, "/")([]() {
        if (!index_cache.empty()) {
            crow::response res(index_cache);
            res.set_header("Content-Type", "text/html");
            return res;
        }
        return crow::response(200, "MAZECHAIN NODE v3.0.0 - [ESTADO: OPERACIONAL]");
    });

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

    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string endereco) {
        double bal = bc.getBalance(endereco);
        crow::json::wvalue x;
        x["address"] = endereco;
        x["balance_mz"] = bal;
        x["balance_mits"] = (long long)(bal * 100000000.0);
        return crow::response(x);
    });

    CROW_ROUTE(app, "/wallet/new")([]() {
        Wallet w; w.create();
        crow::json::wvalue result;
        result["address"] = w.address;
        result["seed"] = w.seed;
        return crow::response(result);
    });

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
            b["reward"] = bc.getBlockReward(block.index); 
            b["tx_count"] = (int)block.transactions.size();
            b["timestamp"] = (long long)block.timestamp;
            block_list.push_back(std::move(b));
        }
        x["blocks"] = std::move(block_list);
        x["tip_height"] = (int)chain.size();
        return crow::response(x);
    });

    CROW_ROUTE(app, "/minerar_agora/<string>")([&bc, &p2p](std::string endereco) {
        try {
            int prev_height = (int)bc.getChain().size();
            bc.mineBlock(endereco);
            Storage::saveChain(bc, ABS_DB_PATH);
            Storage::clearMempool(ABS_MEMPOOL_PATH);
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

    CROW_ROUTE(app, "/p2p/add_node/<string>")([&p2p](std::string node_url) {
        p2p.add_peer(node_url);
        return crow::response(200, "Novo peer adicionado.");
    });

    const char* port_env = std::getenv("PORT");
    int port = (port_env != nullptr) ? std::stoi(port_env) : 10000;

    std::cout << "========================================" << std::endl;
    std::cout << "  MAZECHAIN NODE v3.0.0 ONLINE" << std::endl;
    std::cout << "  Porta API: " << port << std::endl;
    std::cout << "========================================" << std::endl;

    app.port(port).multithreaded().run();
}