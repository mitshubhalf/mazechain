// 1. Corrigindo os includes: Use o nome direto, o compilador busca via -I./include
#include "p2p.h"
#include "storage.h"
#include <iostream>
#include <future>
#include <thread>
#include <curl/curl.h>
#include <nlohmann/json.hpp> 

// CORREÇÃO CRÍTICA: O namespace da nlohmann usa dois pontos (::), não barra (/)
using json = nlohmann::json;

// Callback para o CURL ler a resposta do GET (Sincronização)
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    ((std::string*)userp)->append((char*)contents, totalSize);
    return totalSize;
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

        // Silencia a saída para não sujar o terminal do nó
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);

        curl_easy_perform(curl);

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
}

void P2P::add_peer(std::string peer_url) {
    if (!peer_url.empty() && peer_url.back() == '/') peer_url.pop_back();

    // Supondo que 'peers' seja um std::set ou std::unordered_set definido no p2p.h
    if (peers.find(peer_url) == peers.end()) {
        peers.insert(peer_url);
        std::cout << "🌐 [P2P] Novo peer adicionado: " << peer_url << std::endl;
    }
}

void P2P::broadcast_block(const std::string& block_json) {
    for (const auto& peer : peers) {
        std::string url = peer + "/block";
        // Criando a thread para não travar a mineração enquanto envia
        std::thread([url, block_json]() { 
            send_post_request(url, block_json); 
        }).detach();
    }
}

void P2P::broadcast_transaction(const std::string& tx_json) {
    for (const auto& peer : peers) {
        std::string url = peer + "/tx";
        std::thread([url, tx_json]() { 
            send_post_request(url, tx_json); 
        }).detach();
    }
}

// SINCRONIZAÇÃO: Baixa blocos que você não tem
void P2P::sync_with_peers() {
    if (peers.empty()) {
        std::cout << "⚠️ [P2P] Nenhum peer disponível para sincronização." << std::endl;
        return;
    }

    for (const auto& peer : peers) {
        std::string readBuffer;
        CURL* curl = curl_easy_init();
        if (curl) {
            std::string url = peer + "/blockchain"; 
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

            CURLcode res = curl_easy_perform(curl);
            if (res == CURLE_OK) {
                try {
                    // Aqui o 'json::parse' agora vai funcionar!
                    auto j = json::parse(readBuffer);
                    std::cout << "📥 [P2P] Dados recebidos de " << peer << ". Processando..." << std::endl;

                    // Implementação futura: iterar sobre 'j' e atualizar a Blockchain local

                } catch (const json::parse_error& e) {
                    std::cerr << "❌ [P2P] Erro de parse JSON: " << e.what() << std::endl;
                }
            } else {
                std::cerr << "❌ [P2P] Falha ao conectar com " << peer << ": " << curl_easy_strerror(res) << std::endl;
            }
            curl_easy_cleanup(curl);
        }
    }
}