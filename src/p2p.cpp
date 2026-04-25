#include "../include/p2p.h"
#include "../include/storage.h"
#include <iostream>
#include <future>
#include <thread>
#include <curl/curl.h>
#include <nlohmann/json.hpp> // Certifique-se de ter essa lib para o parse dos blocos

using json = nlohmann/json;

// Callback para o CURL ler a resposta do GET (Sincronização)
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void send_post_request(std::string url, std::string json_data) {
    CURL* curl = curl_easy_init();
    if (curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
}

void P2P::add_peer(std::string peer_url) {
    if (!peer_url.empty() && peer_url.back() == '/') peer_url.pop_back();
    if (peers.find(peer_url) == peers.end()) {
        peers.insert(peer_url);
        std::cout << "🌐 [P2P] Novo peer: " << peer_url << std::endl;
    }
}

void P2P::broadcast_block(const std::string& block_json) {
    for (const auto& peer : peers) {
        std::string url = peer + "/block";
        std::thread([url, block_json]() { send_post_request(url, block_json); }).detach();
    }
}

void P2P::broadcast_transaction(const std::string& tx_json) {
    for (const auto& peer : peers) {
        std::string url = peer + "/tx";
        std::thread([url, tx_json]() { send_post_request(url, tx_json); }).detach();
    }
}

// SINCRONIZAÇÃO: Baixa blocos que você não tem
void P2P::sync_with_peers() {
    if (peers.empty()) return;

    for (const auto& peer : peers) {
        std::string readBuffer;
        CURL* curl = curl_easy_init();
        if (curl) {
            std::string url = peer + "/blockchain"; // Rota que entrega a chain completa
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

            CURLcode res = curl_easy_perform(curl);
            if (res == CURLE_OK) {
                try {
                    auto j = json::parse(readBuffer);
                    std::cout << "📥 [P2P] Sincronizando blocos de " << peer << "..." << std::endl;
                    
                    // Aqui você percorreria o JSON, converteria para Block 
                    // e usaria o blockchain.addBlock() para cada um.
                    // Isso garante que você tenha TODOS os blocos minerados.
                    
                } catch (...) {
                    std::cerr << "❌ [P2P] Erro ao processar dados de sincronização." << std::endl;
                }
            }
            curl_easy_cleanup(curl);
        }
    }
}
