#define CROW_MAIN
#define CROW_ENABLE_CORS
#include "crow_all.h"
#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include "../include/transaction.h"
#include <vector>
#include <string>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <fstream>

// Estrutura de Middleware para garantir a comunicação com o Frontend (CORS)
struct CORS {
    struct context {};
    void before_handle(crow::request& req, crow::response& res, context&) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PATCH, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Origin, Content-Type, Accept, Authorization");
        if (req.method == crow::HTTPMethod::OPTIONS) {
            res.code = 204;
            res.end();
        }
    }
    void after_handle(crow::request&, crow::response& res, context&) {
        res.set_header("Access-Control-Allow-Origin", "*");
    }
};

int main() {
    // Verificação de segurança crítica: sem wordlist, o algoritmo BIP-39 de wallet falha
    std::ifstream check_wordlist("wordlist.txt");
    if (!check_wordlist.is_open()) {
        std::cerr << "❌ ERRO CRÍTICO: wordlist.txt não encontrada!" << std::endl;
        std::cerr << "O nó MazeChain exige a lista de palavras BIP-39 para derivar chaves privadas." << std::endl;
        std::cerr << "Tente: curl -L https://raw.githubusercontent.com/bitcoin/bips/master/bip-0039/english.txt -o wordlist.txt" << std::endl;
        return 1;
    }
    check_wordlist.close();

    // Instanciação da aplicação Crow com o Middleware de CORS ativo
    crow::App<CORS> app; 
    Blockchain bc;
    
    // Tenta carregar o estado persistente da Blockchain do disco
    if(!Storage::loadChain(bc, "data/blockchain.dat")) {
        std::cout << "[INFO] Nenhuma blockchain local encontrada. Iniciando nova rede MazeChain..." << std::endl;
    } else {
        std::cout << "[INFO] Blockchain carregada com sucesso. Altura: " << bc.getChain().size() << std::endl;
    }

    // Rota Raiz: Tenta servir o frontend ou retorna o status textual
    CROW_ROUTE(app, "/")([]() {
        std::ifstream file("index.html");
        if (file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            crow::response res(content);
            res.set_header("Content-Type", "text/html; charset=UTF-8");
            return res;
        }
        return crow::response(200, "MAZECHAIN NODE v2.1 - STATUS: ONLINE (20M MAX SUPPLY)");
    });

    // Rota de Diagnóstico da Rede
    CROW_ROUTE(app, "/status")([&bc]() {
        crow::json::wvalue x;
        x["status"] = "online";
        x["version"] = "2.1.0";
        x["difficulty"] = bc.getDifficulty();
        x["total_supply"] = bc.getTotalSupply();
        x["max_supply"] = 20000000.0;
        x["blocks"] = (int)bc.getChain().size();
        return crow::response(x);
    });

    // Rota de Transação: Recebe JSON com From, To, Amount e Seed para assinar
    CROW_ROUTE(app, "/send").methods(crow::HTTPMethod::POST)([&bc](const crow::request& req) {
        auto x = crow::json::load(req.body);
        crow::json::wvalue result;
        if (!x) return crow::response(400, "Corpo da requisição JSON inválido");

        try {
            std::string from = x["from"].s();
            std::string to = x["to"].s();
            double amount = x["amount"].d();
            std::string seed = x["seed"].s();

            // Chamada interna para o motor da Blockchain (Valida saldo, taxas e assinatura)
            bc.send(from, to, amount, seed);
            
            result["status"] = "success";
            result["message"] = "Transação enviada com sucesso para a mempool";
            return crow::response(200, result);
        } catch (const std::exception& e) {
            result["status"] = "error";
            result["message"] = e.what();
            return crow::response(400, result);
        }
    });

    // Rota do Explorer: Retorna todos os blocos e seus metadados (incluindo Merkle Root)
    CROW_ROUTE(app, "/chain")([&bc]() {
        crow::json::wvalue x;
        std::vector<crow::json::wvalue> block_list;
        for (const auto& block : bc.getChain()) {
            crow::json::wvalue b;
            b["index"] = block.index;
            b["hash"] = block.hash;
            b["prev_hash"] = block.prevHash;
            b["merkle_root"] = block.merkleRoot;
            b["timestamp"] = block.timestamp;
            b["tx_count"] = (int)block.transactions.size();
            block_list.push_back(std::move(b));
        }
        x["blocks"] = std::move(block_list);
        return crow::response(x);
    });

    // Rota de Geração de Carteira: Retorna o endereço e a mnemônica de 12 palavras
    CROW_ROUTE(app, "/wallet/new")([]() {
        Wallet w;
        w.create(); // Gera 12 palavras e deriva as chaves ECDSA secp256k1
        crow::json::wvalue result;
        result["address"] = w.address;
        result["seed"] = w.seed;
        return crow::response(result);
    });

    // Rota de Consulta: Retorna o saldo em unidade MZ e em Mits (Satoshis da MazeChain)
    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string endereco) {
        double bal = bc.getBalance(endereco);
        crow::json::wvalue x;
        x["address"] = endereco;
        x["balance_mz"] = bal;
        x["balance_mits"] = (long long)(bal * 100000000.0);
        return crow::response(x);
    });

    // Rota de Mineração: Gatilho manual para fechar o bloco atual
    CROW_ROUTE(app, "/minerar_agora/<string>")([&bc](std::string endereco) {
        int current_height = (int)bc.getChain().size();
        
        // Inicia o processo de Hashing (Proof of Work)
        bc.mineBlock(endereco);

        // Após minerar, salva o estado e limpa a mempool processada
        Storage::saveChain(bc, "data/blockchain.dat");
        Storage::clearMempool("data/mempool.dat");

        crow::json::wvalue x;
        x["status"] = "success";
        x["block"] = current_height;
        x["reward"] = bc.getBlockReward(current_height);
        return crow::response(x);
    });

    // Configuração de Porta: Prioriza variável de ambiente (Render/Heroku) ou porta 10000
    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 10000;
    
    std::cout << "🚀 MazeChain Node rodando na porta " << port << "..." << std::endl;
    
    app.port(port)
       .bindaddr("0.0.0.0")
       .multithreaded()
       .run();
}
