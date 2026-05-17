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

void P2P::sync_with_peers(Blockchain& bc, const std::string& db_path) {
    if (peers.empty()) return;

    for (const auto& peer : peers) {
        int local_height = (int)bc.getChain().size();
        std::cout << "🔄 [P2P] Sincronizando com " << peer
                  << " (altura local: " << local_height << ")" << std::endl;

        int fetched = 0;
        int limit   = 500;

        while (true) {
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
                Block b({}, {}, {}, {}, 0); // dummy — filled by parse_block
                if (!parse_block(jb, b)) continue;

                if (b.index == (int)bc.getChain().size()) {
                    bc.addBlock(b);
                    Storage::saveBlockToDisk(b, db_path);
                    fetched++;
                    local_height++;
                }
            }

            if ((int)blocks.size() < limit) break; // no more blocks
        }

        if (fetched > 0) {
            std::cout << "✅ [P2P] Sync concluído: +" << fetched
                      << " blocos. Altura: " << bc.getChain().size() << std::endl;
        } else {
            std::cout << "✅ [P2P] Já sincronizado com " << peer << std::endl;
        }
    }
}
