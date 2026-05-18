#include "p2p.h"
#include "blockchain.h"
#include "storage.h"
#include "block.h"
#include "transaction.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Versão do protocolo P2P desta build
static const std::string P2P_PROTOCOL_VERSION = "4.0.0";

// ─── CURL helpers ────────────────────────────────────────────────────────────

static size_t write_cb(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

static void post_async(const std::string& url, const std::string& body) {
    CURL* curl = curl_easy_init();
    if (!curl) return;
    struct curl_slist* hdrs = curl_slist_append(nullptr, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_perform(curl);
    curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);
}

static std::string http_get(const std::string& url) {
    std::string buf;
    CURL* curl = curl_easy_init();
    if (!curl) return buf;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) return "";
    return buf;
}

// ─── Peer management ─────────────────────────────────────────────────────────

void P2P::add_peer(std::string url) {
    if (!url.empty() && url.back() == '/') url.pop_back();
    if (!url.empty() && peers.find(url) == peers.end()) {
        peers.insert(url);
        std::cout << "🌐 [P2P] Peer adicionado: " << url << std::endl;
    }
}

void P2P::remove_peer(const std::string& url) {
    peers.erase(url);
    std::cout << "🌐 [P2P] Peer removido: " << url << std::endl;
}

void P2P::save_peers(const std::string& filename) {
    std::ofstream f(filename, std::ios::trunc);
    if (!f.is_open()) return;
    for (const auto& p : peers) f << p << "\n";
    f.close();
}

void P2P::load_peers(const std::string& filename) {
    std::ifstream f(filename);
    if (!f.is_open()) return;
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty()) add_peer(line);
    }
    f.close();
    std::cout << "🌐 [P2P] " << peers.size() << " peer(s) carregados." << std::endl;
}

// ─── Broadcast ───────────────────────────────────────────────────────────────

void P2P::broadcast_block(const std::string& block_json) {
    for (const auto& peer : peers) {
        std::string url = peer + "/block";
        std::thread([url, block_json]() { post_async(url, block_json); }).detach();
    }
}

void P2P::broadcast_transaction(const std::string& tx_json) {
    for (const auto& peer : peers) {
        std::string url = peer + "/tx";
        std::thread([url, tx_json]() { post_async(url, tx_json); }).detach();
    }
}

// ─── Chain sync ──────────────────────────────────────────────────────────────

// Parse a block from the JSON format used by /blockchain endpoint
static bool parse_block(const json& jb, Block& out_block) {
    try {
        int idx         = jb.at("index").get<int>();
        std::string h   = jb.at("hash").get<std::string>();
        std::string ph  = jb.at("prevHash").get<std::string>();
        long long ts    = jb.at("timestamp").get<long long>();
        int nonce       = jb.value("nonce", 0);
        long eNonce     = (long)jb.value("extraNonce", 0);
        std::string miner = jb.value("miner", "");

        std::vector<Transaction> txs;
        for (const auto& jt : jb.at("txs")) {
            Transaction t;
            t.id        = jt.value("id", "");
            t.signature = jt.value("sig", "");
            t.publicKey = jt.value("pub", "");
            for (const auto& jv : jt.at("vout")) {
                TxOut o;
                o.address = jv.value("addr", "");
                o.amount  = jv.value("amt", 0.0);
                t.vout.push_back(o);
            }
            txs.push_back(t);
        }

        Block b(idx, ph, txs, miner, eNonce);
        b.hash      = h;
        b.timestamp = ts;
        b.nonce     = nonce;
        out_block   = b;
        return true;
    } catch (...) {
        return false;
    }
}

// ── Handshake de Versão ───────────────────────────────────────────────────────
// Antes de sincronizar, verifica se o peer está na mesma rede (mesmo genesis).
bool P2P::version_handshake(const std::string& peer_url, const std::string& local_genesis_hash) {
    std::string resp = http_get(peer_url + "/version");
    if (resp.empty()) {
        std::cerr << "❌ [HANDSHAKE] Sem resposta de " << peer_url << std::endl;
        return false;
    }

    try {
        json j = json::parse(resp);

        std::string peer_network = j.value("network", "");
        std::string peer_genesis = j.value("genesis_hash", "");
        std::string peer_version = j.value("version", "");

        // Verifica mesma rede (mainnet vs testnet)
        // Aceita qualquer versão por ora (forward compatibility)
        if (peer_genesis.empty()) {
            std::cout << "⚠️ [HANDSHAKE] Peer " << peer_url << " não respondeu genesis hash." << std::endl;
            return true; // backward compat com nós antigos sem /version
        }

        if (!local_genesis_hash.empty() && peer_genesis != local_genesis_hash) {
            std::cout << "🚨 [HANDSHAKE] Peer " << peer_url
                      << " está em rede DIFERENTE! Genesis: " << peer_genesis.substr(0, 16) << "..."
                      << " (esperado: " << local_genesis_hash.substr(0, 16) << "...)" << std::endl;
            return false; // Incompatível — rede diferente
        }

        std::cout << "✅ [HANDSHAKE] Peer " << peer_url
                  << " compatível. Versão: " << peer_version
                  << " | Rede: " << peer_network << std::endl;
        return true;

    } catch (...) {
        std::cout << "⚠️ [HANDSHAKE] Resposta inválida de " << peer_url << " — assumindo compatível." << std::endl;
        return true; // Assume compatível se /version não existe (nós legados)
    }
}

// ── Troca de Lista de Peers ───────────────────────────────────────────────────
// Envia nossa lista e adiciona os peers que o outro nó conhece.
void P2P::exchange_peers(const std::string& peer_url) {
    // Monta o body com nossa lista de peers
    json body;
    body["peers"] = json::array();
    for (const auto& p : peers) {
        body["peers"].push_back(p);
    }

    // POST /peers/exchange
    std::string resp;
    CURL* curl = curl_easy_init();
    if (!curl) return;

    std::string post_body = body.dump();
    std::string url = peer_url + "/peers/exchange";
    struct curl_slist* hdrs = curl_slist_append(nullptr, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_body.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 8L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
    curl_easy_perform(curl);
    curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);

    if (resp.empty()) return;

    try {
        json j = json::parse(resp);
        if (!j.contains("peers") || !j["peers"].is_array()) return;

        int added = 0;
        for (const auto& p : j["peers"]) {
            std::string purl = p.get<std::string>();
            if (!purl.empty() && purl != peer_url && peers.find(purl) == peers.end()) {
                add_peer(purl);
                added++;
            }
        }
        if (added > 0) {
            std::cout << "🌐 [P2P] +" << added << " novo(s) peer(s) descobertos via " << peer_url << std::endl;
        }
    } catch (...) {}
}

// ── Bootstrap: conecta seed nodes + arquivo de peers ─────────────────────────
void P2P::bootstrap(const std::string& peers_file, const std::string& local_genesis_hash) {
    // 1. Carrega peers salvos
    load_peers(peers_file);

    // 2. Tenta seed nodes hardcoded (se nenhum peer foi carregado)
    if (peers.empty()) {
        std::cout << "🌱 [P2P] Nenhum peer salvo — tentando seed nodes..." << std::endl;
        for (const auto& seed : MAZECHAIN_SEED_NODES) {
            if (version_handshake(seed, local_genesis_hash)) {
                add_peer(seed);
                exchange_peers(seed);
            }
        }
        if (peers.empty()) {
            std::cout << "🟡 [P2P] Nenhum seed node disponível. Rodando como nó isolado." << std::endl;
        }
    }

    // 3. Troca peers com todos os nós já conhecidos
    for (const auto& peer : peers) {
        exchange_peers(peer);
    }

    save_peers(peers_file);
}

// ── Sincronização com Longest Chain Rule ─────────────────────────────────────
void P2P::sync_with_peers(Blockchain& bc, const std::string& db_path) {
    if (peers.empty()) return;

    // Genesis hash local para handshake
    std::string local_genesis = "";
    auto chain_snap = bc.getChain();
    if (!chain_snap.empty()) local_genesis = chain_snap[0].hash;

    for (const auto& peer : peers) {
        // ── Handshake: verifica compatibilidade antes de sincronizar ──────────
        if (!version_handshake(peer, local_genesis)) {
            std::cout << "⛔ [P2P] Peer " << peer << " ignorado — rede incompatível." << std::endl;
            continue;
        }

        int local_height = (int)bc.getChain().size();
        std::cout << "🔄 [P2P] Sincronizando com " << peer
                  << " (altura local: " << local_height << ")" << std::endl;

        // ── Verifica altura do peer ───────────────────────────────────────────
        std::string status_resp = http_get(peer + "/status");
        int peer_height = 0;
        if (!status_resp.empty()) {
            try {
                json sj = json::parse(status_resp);
                peer_height = sj["chain"]["height"].get<int>();
            } catch (...) {}
        }

        // ── Longest Chain Rule: se peer tem cadeia maior, baixa tudo ─────────
        // Caso de reorg: peer pode ter fork mais longo
        bool try_reorg = (peer_height > local_height + 1);

        if (try_reorg) {
            std::cout << "⚡ [REORG] Peer tem " << peer_height
                      << " blocos vs nossos " << local_height << " — tentando reorg..." << std::endl;

            // Baixa a cadeia completa do peer (até o safety floor)
            std::vector<Block> peer_chain;
            int batch_limit = 500;
            int from = 0;

            while ((int)peer_chain.size() < peer_height) {
                std::string url = peer + "/blockchain?from=" + std::to_string(from)
                                + "&limit=" + std::to_string(batch_limit);
                std::string resp = http_get(url);
                if (resp.empty()) break;

                json j;
                try { j = json::parse(resp); } catch (...) { break; }
                if (!j.contains("blocks") || !j["blocks"].is_array()) break;

                for (const auto& jb : j["blocks"]) {
                    Block b({}, {}, {}, {}, 0);
                    if (parse_block(jb, b)) peer_chain.push_back(b);
                }

                if ((int)j["blocks"].size() < batch_limit) break;
                from += batch_limit;
            }

            if (!peer_chain.empty() && bc.replaceChain(peer_chain)) {
                // Salva a nova cadeia em disco
                Storage::saveChain(bc, db_path);
                std::cout << "✅ [REORG] Cadeia substituída! Nova altura: "
                          << bc.getChain().size() << std::endl;
                continue; // já sincronizado com este peer
            } else {
                std::cout << "⚠️ [REORG] Reorg rejeitado — cadeia do peer inválida ou não mais longa." << std::endl;
            }
        }

        // ── Sincronização normal: baixa apenas blocos novos ──────────────────
        int fetched = 0;
        int limit   = 500;

        while (true) {
            local_height = (int)bc.getChain().size();
            std::string url = peer + "/blockchain?from=" +
                              std::to_string(local_height) + "&limit=" +
                              std::to_string(limit);

            std::string resp = http_get(url);
            if (resp.empty()) {
                std::cerr << "❌ [P2P] Sem resposta de " << peer << std::endl;
                break;
            }

            json j;
            try { j = json::parse(resp); } catch (...) { break; }

            if (!j.contains("blocks") || !j["blocks"].is_array()) break;
            auto& blocks = j["blocks"];
            if (blocks.empty()) break;

            for (const auto& jb : blocks) {
                Block b({}, {}, {}, {}, 0);
                if (!parse_block(jb, b)) continue;

                if (b.index == (int)bc.getChain().size()) {
                    bc.addBlock(b);
                    Storage::saveBlockToDisk(b, db_path);
                    fetched++;
                }
            }

            if ((int)blocks.size() < limit) break;
        }

        if (fetched > 0) {
            std::cout << "✅ [P2P] Sync concluído: +" << fetched
                      << " blocos. Altura: " << bc.getChain().size() << std::endl;
        } else {
            std::cout << "✅ [P2P] Já sincronizado com " << peer << std::endl;
        }

        // Descobre novos peers após sincronizar
        exchange_peers(peer);
    }
}
