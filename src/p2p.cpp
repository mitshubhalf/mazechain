#include "../include/p2p.h"
#include <iostream>
#include <future> // Para broadcast assíncrono (não travar a mineração)

// Nota: Para usar CURL, você precisará instalar a libcurl no ambiente
// No Replit/Nix costuma ser: libcurl.dev
#include <curl/curl.h> 

void P2P::add_peer(std::string peer_url) {
    // Evita adicionar o próprio nó ou duplicados
    if (peers.find(peer_url) == peers.end()) {
        peers.insert(peer_url);
        std::cout << "🌐 [P2P] Conectado ao peer: " << peer_url << std::endl;
    }
}

// Função auxiliar interna para fazer o POST real
void send_post_request(std::string url, std::string json_data) {
    CURL* curl = curl_easy_init();
    if (curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L); // Timeout de 5 segundos

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "❌ [P2P] Falha ao enviar para " << url << ": " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
}

void P2P::broadcast_block(const std::string& block_json) {
    if (peers.empty()) {
        std::cout << "📡 [P2P] Nenhum peer conectado para propagar bloco." << std::endl;
        return;
    }

    for (const auto& peer : peers) {
        std::cout << "📡 [P2P] Propagando bloco para: " << peer << std::endl;
        // Rodar em uma thread separada para o minerador não ficar esperando a rede
        std::async(std::launch::async, send_post_request, peer + "/block", block_json);
    }
}

void P2P::broadcast_transaction(const std::string& tx_json) {
    for (const auto& peer : peers) {
        std::cout << "📡 [P2P] Propagando transação para: " << peer << std::endl;
        std::async(std::launch::async, send_post_request, peer + "/tx", tx_json);
    }
}