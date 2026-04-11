#include "crow_all.h"
#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include <vector>
#include <string>
#include <cstdlib>
#include <set>
#include <sstream>
#include <iostream>

std::set<std::string> peers;

int main() {
    crow::SimpleApp app;
    Blockchain bc;
    
    // Tenta carregar a blockchain do arquivo local
    Storage::loadChain(bc, "data/blockchain.dat");

    // ROTA RAIZ - Diagnóstico rápido
    CROW_ROUTE(app, "/")([]() {
        return "MAZECHAIN NODE v1.0 - STATUS: ONLINE";
    });

    // ROTA: GERAR NOVA CARTEIRA
    // Gera o endereço MZ e a Seed de 12 palavras
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
        ss << "AVISO: ANOTE AS PALAVRAS AGORA!\n";
        ss << "====================================";
        
        crow::response res(ss.str());
        res.add_header("Access-Control-Allow-Origin", "*"); // Libera para o site
        return res;
    });

    // ROTA: ESTATISTICAS (Altura da rede e Dificuldade)
    CROW_ROUTE(app, "/stats")([&bc]() {
        crow::json::wvalue x;
        x["height"] = (int)bc.getChain().size();
        x["difficulty"] = bc.getDifficulty();
        x["peers"] = (int)peers.size();
        
        crow::response res(x);
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Content-Type", "application/json");
        return res;
    });

    // ROTA: MINERAR AGORA (Acionada pelo botão do site)
    CROW_ROUTE(app, "/minerar_agora/<string>")
    ([&bc](std::string endereco) {
        // Executa a Prova de Trabalho (PoW) no Servidor
        bc.mineBlock(endereco);
        
        // Salva imediatamente após minerar
        Storage::saveChain(bc, "data/blockchain.dat");

        crow::json::wvalue x;
        x["status"] = "success";
        x["blocos_agora"] = (int)bc.getChain().size();
        x["minerador"] = endereco;
        
        crow::response res(x);
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });

    // ROTA: EXPLORADOR DE BLOCOS (JSON)
    CROW_ROUTE(app, "/chain")([&bc]() {
        std::vector<crow::json::wvalue> blocks_json;
        for (const auto& b : bc.getChain()) {
            crow::json::wvalue block;
            block["index"] = b.index;
            block["hash"] = b.hash;
            block["prevHash"] = b.prevHash;
            block["timestamp"] = (long long)b.timestamp;
            block["nonce"] = b.nonce;
            blocks_json.push_back(std::move(block));
        }
        
        // CORREÇÃO: Usar { } impede que o compilador confunda o objeto com uma função
        crow::response res{crow::json::wvalue(blocks_json)}; 
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Content-Type", "application/json");
        return res;
    });

    // CONFIGURAÇÃO DA PORTA DINÂMICA (PARA O RENDER)
    const char* port_ptr = std::getenv("PORT");
    int port = (port_ptr != nullptr) ? std::stoi(port_ptr) : 10000;
    
    std::cout << "MazeChain rodando na porta: " << port << std::endl;
    app.port(port).multithreaded().run();
}
