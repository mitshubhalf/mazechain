#include "../include/p2p.h"
#include <iostream>

// Comentamos o CURL para evitar erro de falta de biblioteca no Replit
// #include <curl/curl.h> 

void P2P::add_peer(std::string peer_url) {
    peers.insert(peer_url);
    std::cout << "🌐 Novo peer adicionado: " << peer_url << std::endl;
}

void P2P::broadcast_block(const std::string& block_json) {
    for (const auto& peer : peers) {
        std::cout << "📡 Propagando bloco para: " << peer << "..." << std::endl;
        // A lógica de envio real via rede (POST) será implementada aqui futuramente
        // por enquanto, o nó apenas simula a propagação via log.
        (void)block_json; // Evita aviso de variável não utilizada
    }
}

// Caso seu p2p.h declare a função de transação, adicione-a para não dar erro de linkagem:
void P2P::broadcast_transaction(const std::string& tx_json) {
    for (const auto& peer : peers) {
        std::cout << "📡 Propagando transação para: " << peer << "..." << std::endl;
        (void)tx_json;
    }
}