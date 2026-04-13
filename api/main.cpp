#include "crow_all.h"
#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include <vector>
#include <string>
#include <cstdlib>
#include <sstream>
#include <iostream>

// Função para habilitar comunicação com o Frontend (CORS) - COMPLETA
void add_cors(crow::response& res) {
    res.add_header("Access-Control-Allow-Origin", "*");
    res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.add_header("Access-Control-Allow-Headers", "Content-Type");
}

int main() {
    crow::SimpleApp app;
    Blockchain bc;
    
    // Carrega a blockchain do armazenamento persistente
    Storage::loadChain(bc, "data/blockchain.dat");

    // ROTA RAIZ
    CROW_ROUTE(app, "/")([]() {
        return "MAZECHAIN NODE v1.1 - STATUS: ONLINE";
    });

    // ROTA OPTIONS PARA O ENDPOINT /send (CORS Preflight)
    CROW_ROUTE(app, "/send").methods(crow::HTTPMethod::OPTIONS)([]() {
        crow::response res(204);
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");
        return res;
    });

    // ROTA DE ENVIO DE TRANSAÇÕES - COMPLETA COM TRATAMENTO DE ERRO
    CROW_ROUTE(app, "/send").methods(crow::HTTPMethod::POST)([&bc](const crow::request& req) {
        auto x = crow::json::load(req.body);
        crow::json::wvalue result;
        
        if (!x) {
            result["status"] = "error";
            result["message"] = "JSON Invalido";
            crow::response res(400, result);
            add_cors(res);
            return res;
        }

        try {
            if (!x.has("from") || !x.has("to") || !x.has("amount") || !x.has("seed")) {
                throw std::runtime_error("Campos obrigatorios ausentes (from, to, amount, seed)");
            }

            std::string from = x["from"].s();
            std::string to = x["to"].s();
            double amount = x["amount"].d();
            std::string seed = x["seed"].s();

            // Executa a lógica de envio na blockchain
            bc.send(from, to, amount, seed);

            result["status"] = "success";
            result["message"] = "Transacao enviada para Mempool com sucesso";
            crow::response res(200, result);
            add_cors(res);
            return res;
        } catch (const std::exception& e) {
            result["status"] = "error";
            result["message"] = e.what();
            crow::response res(400, result);
            add_cors(res);
            return res;
        }
    });

    // ROTA PARA GERAR NOVA CARTEIRA
    CROW_ROUTE(app, "/wallet/new")([]() {
        Wallet w;
        w.create();
        std::stringstream ss;
        ss << "ENDERECO (MZ):\n" << w.address << "\n\nSEED:\n>> " << w.seed << " <<";
        crow::response res(ss.str());
        add_cors(res);
        return res;
    });

    // ROTA PARA CONSULTAR SALDO DE UM ENDEREÇO
    CROW_ROUTE(app, "/balance/<string>")([&bc](std::string endereco) {
        double saldo = bc.getBalance(endereco);
        crow::json::wvalue x;
        x["address"] = endereco;
        x["balance"] = saldo;
        crow::response res{x};
        add_cors(res);
        return res;
    });

    // ROTA DE MINERAÇÃO MANUAL VIA API
    CROW_ROUTE(app, "/minerar_agora/<string>")([&bc](std::string endereco) {
        crow::json::wvalue x;
        if(endereco.substr(0, 2) != "MZ") {
            x["status"] = "error";
            x["message"] = "Endereco invalido. Deve comecar com MZ";
            crow::response res(400, x);
            add_cors(res);
            return res;
        }
        bc.mineBlock(endereco);
        Storage::saveChain(bc, "data/blockchain.dat"); 
        
        x["status"] = "success";
        x["total_blocos"] = (int)bc.getChain().size();
        x["dificuldade_atual"] = bc.getDifficulty();
        
        crow::response res(x);
        add_cors(res);
        return res;
    });

    // ROTA PARA EXPLORADOR DE BLOCOS (CHAIN COMPLETA)
    CROW_ROUTE(app, "/chain")([&bc]() {
        std::vector<crow::json::wvalue> blocks_json;
        for (const auto& b : bc.getChain()) {
            crow::json::wvalue block;
            block["index"] = b.index;
            block["hash"] = b.hash;
            block["prevHash"] = b.prevHash;
            block["timestamp"] = (long long)b.timestamp;
            block["nonce"] = b.nonce;
            block["tx_count"] = (int)b.transactions.size();
            blocks_json.push_back(std::move(block));
        }
        crow::response res{crow::json::wvalue(blocks_json)};
        add_cors(res);
        return res;
    });

    // ROTA PARA VER TRANSAÇÕES PENDENTES NA MEMPOOL
    CROW_ROUTE(app, "/mempool")([&bc]() {
        std::vector<crow::json::wvalue> txs_json;
        auto pending = bc.getMempool();
        for (const auto& tx : pending) { 
            crow::json::wvalue j_tx;
            j_tx["id"] = tx.id;
            
            if (!tx.vout.empty()) {
                for(const auto& out : tx.vout) {
                    if (out.amount > 0) {
                        j_tx["to"] = out.address;
                        j_tx["amount"] = out.amount;
                    } else {
                        j_tx["from"] = out.address;
                    }
                }
            }
            txs_json.push_back(std::move(j_tx));
        }
        crow::response res{crow::json::wvalue(txs_json)};
        add_cors(res);
        return res;
    });

    // ROTA DE STATUS GERAL DO NODE
    CROW_ROUTE(app, "/status")([&bc]() {
        crow::json::wvalue x;
        x["moeda"] = "MazeCoin";
        x["simbolo"] = "MZ";
        x["versao"] = "1.1.2";
        x["total_blocos"] = (int)bc.getChain().size();
        x["mempool_size"] = (int)bc.getMempool().size();
        // Nota: Certifique-se que getTotalSupply() existe no seu blockchain.h
        // Se não existir, use bc.getChain().size() ou outra métrica.
        x["circulacao_total"] = bc.getChain().size() * 1000; 
        x["dificuldade"] = bc.getDifficulty();
        x["status_rede"] = "ONLINE";
        
        crow::response res(x);
        add_cors(res);
        return res;
    });

    // INICIALIZAÇÃO DO SERVIDOR
    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 10000;
    
    std::cout << "Node MazeChain ativo na porta: " << port << std::endl;
    app.port(port).multithreaded().run();
}
