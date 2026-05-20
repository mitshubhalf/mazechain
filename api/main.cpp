#define CROW_MAIN
#define CROW_ENABLE_CORS
#include "crow_all.h"
#include "blockchain.h"
#include "storage.h"
#include "wallet.h"
#include "transaction.h"
#include "p2p.h"
#include "node_manager.h"
#include "crypto.h"
#include "hd_wallet.h"
#include "multisig.h"
#include "testnet.h"
#include "mempool_expiry.h"
#include "db_integrity.h"

// ── Novos módulos (~150 features) ───────────────────────────────────────────
#include "script_vm.h"
#include "locktime.h"
#include "bech32.h"
#include "fee_estimation.h"
#include "orphan_pool.h"
#include "ban_score.h"
#include "dos_protection.h"
#include "wallet_features.h"
#include "merkle_proof.h"
#include "schnorr.h"
#include "logging_system.h"
#include "metrics.h"
#include "config_file.h"
#include "version_bits.h"
#include "inventory.h"
#include "regtest.h"
#include "bloom_filter.h"
#include "stratum.h"
#include "extra_nonce.h"
#include "rpc_auth.h"
#include "ec_recovery.h"
#include "peer_eviction.h"
#include "sse.h"

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
#include <atomic> 
#include <csignal> 
#include <sys/stat.h>
#include <mutex>
#include <chrono>
#include <curl/curl.h>

#ifndef _WIN32
#include <termios.h>
#include <unistd.h>
#define MKDIR(path) mkdir(path, 0777)
#else
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#endif

// --- CONTROLE GLOBAL ---
std::atomic<bool> is_mining{false};          
std::atomic<bool> global_keep_running{true}; 
extern std::mutex g_blockchain_mutex; // Usa o mutex definido no blockchain.cpp

// ── Per-node paths (overridable via env vars for multi-node clusters) ─────────
static std::string _resolveDataDir() {
    const char* env = std::getenv("MAZE_DATADIR");
    return env ? std::string(env) : "data";
}
static std::string _resolveWalletFile() {
    const char* env = std::getenv("MAZE_WALLET_FILE");
    if (env) return std::string(env);
    return _resolveDataDir() + "/wallet.dat";
}
static std::string _resolvePeersFile() {
    const char* env = std::getenv("MAZE_PEERS_FILE");
    if (env) return std::string(env);
    return _resolveDataDir() + "/peers.dat";
}

const std::string ABS_DATA_PATH   = _resolveDataDir();
const std::string ABS_WALLET_PATH = _resolveWalletFile();
const std::string MEMPOOL_PATH    = _resolveDataDir() + "/mempool.dat";
const std::string DB_PATH         = _resolveDataDir() + "/blockchain.dat";

// --- SIGNAL HANDLER ---
void signal_handler(int signum) {
    std::cout << "\n[SISTEMA] Sinal (" << signum << ") recebido. Desligando..." << std::endl;
    global_keep_running = false;
    is_mining = false;
    exit(signum);
}

// ─── Block JSON serialization (for P2P broadcast and /blockchain endpoint) ───

static std::string esc_json(const std::string& s) {
    std::string o; o.reserve(s.size());
    for (char c : s) {
        if      (c == '"')  o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else if (c == '\n') o += "\\n";
        else if (c == '\r') o += "\\r";
        else                o += c;
    }
    return o;
}

static std::string block_to_json_str(const Block& b) {
    std::string j = "{";
    j += "\"index\":"      + std::to_string(b.index)      + ",";
    j += "\"hash\":\""     + esc_json(b.hash)              + "\",";
    j += "\"prevHash\":\"" + esc_json(b.prevHash)          + "\",";
    j += "\"timestamp\":"  + std::to_string(b.timestamp)  + ",";
    j += "\"nonce\":"      + std::to_string(b.nonce)       + ",";
    j += "\"miner\":\""    + esc_json(b.minerAddress)      + "\",";
    j += "\"extraNonce\":"  + std::to_string(b.extraNonce)  + ",";
    j += "\"txs\":[";
    for (size_t i = 0; i < b.transactions.size(); i++) {
        if (i) j += ",";
        const auto& tx = b.transactions[i];
        j += "{\"id\":\""  + esc_json(tx.id)         + "\",";
        j += "\"sig\":\""  + esc_json(tx.signature)  + "\",";
        j += "\"pub\":\""  + esc_json(tx.publicKey)  + "\",";
        j += "\"vout\":[";
        for (size_t k = 0; k < tx.vout.size(); k++) {
            if (k) j += ",";
            j += "{\"addr\":\"" + esc_json(tx.vout[k].address) + "\",";
            j += "\"amt\":"     + std::to_string(tx.vout[k].amount) + "}";
        }
        j += "]}";
    }
    j += "]}";
    return j;
}

// ─── Block parsing from Crow JSON (for POST /block) ──────────────────────────

static bool block_from_crow_json(const crow::json::rvalue& body, Block& out) {
    try {
        int idx           = body["index"].i();
        std::string hash  = (std::string)body["hash"].s();
        std::string ph    = (std::string)body["prevHash"].s();
        long long ts      = (long long)body["timestamp"].i();
        int nonce         = (int)body["nonce"].i();
        long eNonce       = (long)body["extraNonce"].i();
        std::string miner = (std::string)body["miner"].s();

        std::vector<Transaction> txs;
        for (const auto& jt : body["txs"]) {
            Transaction t;
            t.id        = (std::string)jt["id"].s();
            t.signature = (std::string)jt["sig"].s();
            t.publicKey = (std::string)jt["pub"].s();
            for (const auto& jv : jt["vout"]) {
                TxOut o;
                o.address = (std::string)jv["addr"].s();
                o.amount  = jv["amt"].d();
                t.vout.push_back(o);
            }
            txs.push_back(t);
        }

        Block b(idx, ph, txs, miner, eNonce);
        b.hash      = hash;
        b.timestamp = ts;
        b.nonce     = nonce;
        out = b;
        return true;
    } catch (...) { return false; }
}

// --- FUNÇÃO DE MINERAÇÃO ---
void async_mine(Blockchain& bc, P2P& p2p, std::string addr) {
    bool expected = false;
    if (!is_mining.compare_exchange_strong(expected, true)) {
        return;
    }

    try {
        int altura_antes = (int)bc.getChain().size();
        std::cout << "\n[MOTOR] ⛏️  Minerando Bloco #" << altura_antes 
                  << " para: " << addr << std::endl;

        // Notify frontend: mining started
        SSE::pushMiningStart(altura_antes, addr);

        auto t_start = std::chrono::steady_clock::now();
        bc.mineBlock(addr);
        auto t_end   = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

        int altura_depois = (int)bc.getChain().size();
        if (altura_depois > altura_antes) {
            const auto& chain = bc.getChain();
            const Block& blk  = chain.back();

            std::cout << "[MOTOR] ✅ Bloco " << (altura_depois - 1)
                      << " | Hash: " << blk.hash.substr(0, 20) << "..."
                      << " | Tempo: " << std::fixed << std::setprecision(1) << ms << "ms"
                      << std::endl;

            // SSE: mining done + new block event (frontend updates live)
            SSE::pushMiningDone(altura_depois - 1, blk.hash, true, ms);

            double reward = bc.getBlockReward(altura_depois - 1);
            double supply = bc.getTotalSupply();
            int    diff   = bc.getDifficulty();
            int    txc    = (int)blk.transactions.size();
            SSE::pushBlock(altura_depois - 1, blk.hash, blk.minerAddress,
                           reward, txc, supply, diff);

            // Broadcast block to peers
            std::string block_json = block_to_json_str(blk);
            p2p.broadcast_block(block_json);

        } else {
            SSE::pushMiningDone(altura_antes, "", false, ms);
            std::cerr << "[AVISO] Bloco rejeitado ou já minerado por outra thread." << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "[ERRO NA MINERAÇÃO] " << e.what() << std::endl;
    }

    is_mining.store(false);
}

// 🔥 LOOP DE MINERAÇÃO AUTOMÁTICA
// Retry imediato: se um bloco for inválido/rejeitado, minera o mesmo height novamente
// sem pausa — garante que blocos inválidos nunca avancem na cadeia.
void worker_mineracao_continua(Blockchain& bc, P2P& p2p, std::string local_addr) {
    std::cout << "[SISTEMA] Motor automático iniciado. Recompensas para: " << local_addr << std::endl;
    while (global_keep_running) {
        if (!is_mining.load()) {
            async_mine(bc, p2p, local_addr);
        }
        // 100ms: permite retry quase imediato após rejeição, sem busy-loop
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// --- CORS ---
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
    void after_handle(crow::request&, crow::response&, context&) {}
};

int main(int argc, char* argv[]) {
    // Garante que o diretório de dados existe
    MKDIR(ABS_DATA_PATH.c_str());

    if (argc > 1) {
        std::string cmd = argv[1];

        // Silenciar outputs internos para comandos CLI limpos
        std::streambuf* orig_buf = std::cout.rdbuf();
        std::stringstream ss_silence; 
        std::cout.rdbuf(ss_silence.rdbuf()); 

        auto bc_cli = std::make_unique<Blockchain>();

        // --- MELHORIA: CARREGAR DADOS ANTES DE EXECUTAR CLI ---
        Storage::loadChain(*bc_cli, DB_PATH);

        auto p2p_cli = std::make_unique<P2P>();
        auto wallet_cli = std::make_unique<Wallet>();
        wallet_cli->loadFromFile(ABS_WALLET_PATH);

        std::cout.rdbuf(orig_buf); 

        if (cmd == "stats") {
            std::cout << "\n=== MAZECHAIN EXPLORER (CLI) ===" << std::endl;
            std::cout << "⛓️  Altura da Chain: " << bc_cli->getChain().size() << std::endl;
            if (!bc_cli->getChain().empty()) {
                std::cout << "💎 Ultimo Bloco:    " << bc_cli->getChain().back().hash << std::endl;
                std::cout << "⏱️  Timestamp:      " << bc_cli->getChain().back().timestamp << std::endl;
            }
            return 0;
        }

        if (cmd == "balance") {
            std::string target_addr = (argc < 3) ? wallet_cli->getAddress() : argv[2];
            double balance = bc_cli->getBalance(target_addr);
            std::cout << "💰 Saldo Atual de [" << target_addr << "]: " << std::fixed << std::setprecision(8) << balance << " MZ" << std::endl;
            return 0;
        }

        if (cmd == "history" || cmd == "transactions") {
            std::string target_addr = (argc < 3) ? wallet_cli->getAddress() : argv[2];
            std::cout << "\n=== HISTÓRICO DE TRANSAÇÕES: " << target_addr << " ===" << std::endl;
            bc_cli->printAddressHistory(target_addr);
            return 0;
        }

        if (cmd == "mine") {
            // mine looping [addr]
            if (argc >= 3 && std::string(argv[2]) == "looping") {
                std::string target_addr = (argc < 4) ? wallet_cli->getAddress() : argv[3];
                std::cout << "⛏️  Mineração em loop iniciada para: " << target_addr << std::endl;
                std::cout << "   (pressione Ctrl+C para parar)" << std::endl;
                while (true) {
                    async_mine(*bc_cli, *p2p_cli, target_addr);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                return 0;
            }
            // mine [addr]  — minera um único bloco
            std::string target_addr = (argc < 3) ? wallet_cli->getAddress() : argv[2];
            std::cout << "⛏️  Iniciando mineração manual para: " << target_addr << std::endl;
            async_mine(*bc_cli, *p2p_cli, target_addr);
            return 0;
        }

        if (cmd == "send") {
            if (argc < 6) {
                std::cout << "Uso: ./mazechain send [origem] [destino] [quantia] [seed]" << std::endl;
                return 1;
            }
            std::string from_addr = argv[2];
            std::string to_addr = argv[3];
            std::string amount_str = argv[4];
            std::string seed_phrase = argv[5];
            double amount = 0.0;
            try { amount = std::stod(amount_str); } catch (...) {
                std::cout << "🚨 Erro: Quantia inválida '" << amount_str << "'" << std::endl;
                return 1;
            }
            try {
                bc_cli->send(from_addr, to_addr, amount, seed_phrase);
                std::cout << "✅ Transação enviada para o Mempool com sucesso!" << std::endl;
            } catch (const std::exception& e) {
                std::cout << "❌ Erro ao processar transação: " << e.what() << std::endl;
            }
            return 0;
        }

        if (cmd == "address") {
            std::cout << "Your MazeChain Address: " << wallet_cli->getAddress() << std::endl;
            return 0;
        }

        if (cmd == "chain") {
            std::cout << "\n=== LISTA DE BLOCOS (REDE MAZECHAIN) ===" << std::endl;
            for (const auto& block : bc_cli->getChain()) {
                std::cout << "#" << std::setw(5) << std::left << block.index 
                          << " | Hash: " << block.hash.substr(0,20) << "..." 
                          << " | Miner: " << block.minerAddress << std::endl;
            }
            return 0;
        }

        if (cmd == "verify") {
            std::cout << "🔍 Verificando integridade da database..." << std::endl;
            if (bc_cli->isChainValid()) {
                std::cout << "✅ Blockchain integra. Blocos verificados: " << bc_cli->getChain().size() << std::endl;
            } else {
                std::cout << "🚨 Erro na validação da cadeia!" << std::endl;
            }
            return 0;
        }

        if (cmd == "mempool") {
            if (argc >= 3 && std::string(argv[2]) == "purge") {
                int removed = MempoolExpiry::purgeExpired(MEMPOOL_PATH);
                std::cout << "✅ " << removed << " transação(ões) expirada(s) removida(s) do mempool." << std::endl;
                return 0;
            }
            auto pending = Storage::loadMempool(MEMPOOL_PATH);
            std::cout << "\n📦 Mempool Atual: " << pending.size() << " transações pendentes." << std::endl;
            if (pending.empty()) {
                std::cout << "   (Vazio — transações são mineradas automaticamente em segundos)" << std::endl;
            }
            for (size_t i = 0; i < pending.size(); i++) {
                const auto& tx = pending[i];
                std::cout << "\n  [" << (i+1) << "] ID: " << tx.id.substr(0,16) << "..." << std::endl;
                if (tx.vout.size() >= 2) {
                    std::cout << "      De:     " << tx.vout[1].address << std::endl;
                    std::cout << "      Para:   " << tx.vout[0].address << std::endl;
                    std::cout << "      Valor:  " << std::fixed << std::setprecision(4) << tx.vout[0].amount << " MZ" << std::endl;
                    double fee = std::abs(tx.vout[1].amount) - tx.vout[0].amount;
                    std::cout << "      Taxa:   " << fee << " MZ" << std::endl;
                } else if (!tx.vout.empty()) {
                    std::cout << "      Para:   " << tx.vout[0].address << std::endl;
                    std::cout << "      Valor:  " << tx.vout[0].amount << " MZ" << std::endl;
                }
            }
            return 0;
        }

        if (cmd == "wallet") {
            std::cout.rdbuf(orig_buf);
            std::string subcmd = (argc >= 3) ? std::string(argv[2]) : "show";
            if (subcmd == "new") {
                Wallet w;
                w.generateKey();
                std::cout << "\n======================================" << std::endl;
                std::cout << "  NOVA CARTEIRA MAZECHAIN GERADA" << std::endl;
                std::cout << "======================================" << std::endl;
                std::cout << "  Endereço : " << w.address << std::endl;
                std::cout << "  Seed     : " << w.seed << std::endl;
                std::cout << "======================================" << std::endl;
                std::cout << "  ⚠️  Guarde sua Seed em lugar seguro!" << std::endl;
                std::cout << "  Nunca a compartilhe com ninguém." << std::endl;
            } else {
                std::cout << "Endereço do nó : " << wallet_cli->getAddress() << std::endl;
                std::cout << "Uso: ./mazechain wallet new   — cria uma nova carteira" << std::endl;
            }
            return 0;
        }

        if (cmd == "checkpoints") {
            std::cout.rdbuf(orig_buf);
            std::cout << "\n=== CHECKPOINTS MAZECHAIN ===" << std::endl;
            std::cout << "  Bloco #0 (Gênesis): 8c6a56813b90f33cce1ec54afba2ef689fcccf24d7a35521ff02b57bd3075309" << std::endl;
            std::ifstream cpf("data/checkpoints.dat");
            if (cpf.is_open()) {
                int h; std::string hash;
                while (cpf >> h >> hash) {
                    if (h > 0) {
                        std::cout << "  Bloco #" << h << " (Halving): " << hash << std::endl;
                    }
                }
            } else {
                std::cout << "  (Nenhum checkpoint de halving ainda registrado)" << std::endl;
            }
            return 0;
        }

        if (cmd == "help" || cmd == "--help") {
            std::cout << "\n=== MAZECHAIN CLI - COMANDOS DISPONÍVEIS ===" << std::endl;
            std::cout << "  mine [addr]                    : Minera um único bloco" << std::endl;
            std::cout << "  mine looping [addr]            : Minera em loop contínuo (Ctrl+C para parar)" << std::endl;
            std::cout << "  send [from] [to] [amt] [seed]  : Envia moedas validando com a seed" << std::endl;
            std::cout << "  balance [addr]                 : Consulta o saldo" << std::endl;
            std::cout << "  history [addr]                 : Lista transações enviadas e recebidas" << std::endl;
            std::cout << "  wallet new                     : Cria uma nova carteira (seed + endereço)" << std::endl;
            std::cout << "  wallet show                    : Mostra endereço do nó atual" << std::endl;
            std::cout << "  address                        : Mostra seu endereço de recebimento" << std::endl;
            std::cout << "  stats                          : Exibe altura e último hash" << std::endl;
            std::cout << "  chain                          : Lista todos os blocos minerados" << std::endl;
            std::cout << "  verify                         : Valida a integridade dos dados" << std::endl;
            std::cout << "  mempool                        : Exibe transações pendentes" << std::endl;
            std::cout << "  mempool purge                  : Remove transações expiradas (>24h) do mempool" << std::endl;
            std::cout << "  checkpoints                    : Lista todos os pontos seguros registrados" << std::endl;
            return 0;
        }

        std::cout << "❌ Comando desconhecido. Use './mazechain help'" << std::endl;
        return 1;
    }

    // --- MODO NÓ COMPLETO ---
    std::signal(SIGINT, signal_handler);

    auto bc_ptr = std::make_unique<Blockchain>();
    auto p2p_ptr = std::make_unique<P2P>();
    Blockchain& bc = *bc_ptr;
    P2P& p2p = *p2p_ptr;

    // Blockchain já carregada pelo constructor via data/blocks/blk00000.dat
    std::cout << "[SISTEMA] Blockchain inicializada. Altura: " << bc.getChain().size() << " blocos." << std::endl;

    Wallet miner_wallet;
    if (!miner_wallet.loadFromFile(ABS_WALLET_PATH)) {
        std::cout << "\n[CARTEIRA] Nenhuma carteira detectada. Gerando chaves..." << std::endl;
        miner_wallet.generateKey();
        if (miner_wallet.isValid()) {
            std::cout << "\n==================================================" << std::endl;
            std::cout << "⚠️  MAZECHAIN: CARTEIRA GERADA" << std::endl;
            std::cout << "ENDEREÇO: " << miner_wallet.getAddress() << std::endl;
            std::cout << "==================================================\n" << std::endl;
            miner_wallet.saveToFile(ABS_WALLET_PATH);
        }
    }

    // MAZE_MINEADDR env var: lets each node mine to its own wallet
    std::string local_miner_address;
    {
        const char* env_addr = std::getenv("MAZE_MINEADDR");
        if (env_addr && std::strlen(env_addr) > 0) {
            local_miner_address = std::string(env_addr);
            std::cout << "🌐 Endereço de mineração (env): " << local_miner_address << std::endl;
        } else {
            local_miner_address = miner_wallet.getAddress();
            std::cout << "💳 Endereço do Nó Ativo: " << local_miner_address << std::endl;
        }
    }

    NodeManager node_manager(bc, p2p);

    // Bootstrap: carrega peers + tenta seed nodes + troca listas
    {
        std::string genesis_hash = "";
        auto ch = bc.getChain();
        if (!ch.empty()) genesis_hash = ch[0].hash;
        std::string peers_path = _resolvePeersFile();

        std::thread([&bc, &p2p, genesis_hash, peers_path](){
            std::this_thread::sleep_for(std::chrono::seconds(3));
            p2p.bootstrap(peers_path, genesis_hash);
            if (!p2p.peers.empty()) {
                SSE::pushNodeStatus((int)bc.getChain().size(),
                    bc.getTotalSupply(), (int)p2p.peers.size(),
                    0, bc.getDifficulty());
                p2p.sync_with_peers(bc, DB_PATH);
            }
        }).detach();
    }

    // Motor automático DESATIVADO — mineração apenas por comandos manuais:
    //   ./mazechain mine [address]          — minera um bloco
    //   ./mazechain mine looping [address]  — minera em loop

    crow::App<CORS> app;

    CROW_ROUTE(app, "/api/my_address")
    ([&miner_wallet](){
        crow::json::wvalue x;
        x["address"] = miner_wallet.getAddress();
        return x;
    });

    CROW_ROUTE(app, "/api/send").methods(crow::HTTPMethod::POST)
    ([&bc, &miner_wallet](const crow::request& req){
        auto j = crow::json::load(req.body);
        if (!j) return crow::response(400, "JSON Inválido");

        std::string to = j["to"].s();
        double amount = j["amount"].d();
        std::string seed = j.has("seed") ? (std::string)j["seed"].s() : "";

        if (bc.getBalance(miner_wallet.getAddress()) < amount) {
            return crow::response(400, "Saldo insuficiente no nó.");
        }

        try {
            bc.send(miner_wallet.getAddress(), to, amount, seed);
            crow::json::wvalue res;
            res["status"] = "success";
            return crow::response(res);
        } catch (const std::exception& e) {
            return crow::response(500, e.what());
        }
    });

    // ── SSE: real-time event stream ────────────────────────────────────────────
    // Frontend connects via EventSource("/events") and receives live updates.
    // Uses long-polling: waits up to 25s for new events, then returns so the
    // browser reconnects — this works perfectly with Crow's sync routing.
    CROW_ROUTE(app, "/events")
    ([&bc, &p2p](const crow::request& req) {
        // Parse Last-Event-ID header (browser sends automatically on reconnect)
        uint64_t afterId = 0;
        std::string lei = req.get_header_value("Last-Event-ID");
        if (!lei.empty()) {
            try { afterId = (uint64_t)std::stoull(lei); } catch (...) {}
        }
        // Also accept ?since=<id> query param
        auto since_param = req.url_params.get("since");
        if (since_param) {
            try { afterId = (uint64_t)std::stoull(since_param); } catch (...) {}
        }

        std::string body = SSE::waitForEvents(afterId, 5000);
        // Tell browser to retry in 2s on disconnect
        body = "retry: 2000\n" + body;

        crow::response res(200, body);
        res.set_header("Content-Type",      "text/event-stream");
        res.set_header("Cache-Control",     "no-cache, no-store");
        res.set_header("Connection",        "keep-alive");
        res.set_header("X-Accel-Buffering", "no");
        return res;
    });

    // ── SSE: latest event ID (for frontend to bootstrap) ──────────────────────
    CROW_ROUTE(app, "/events/latest")
    ([&bc, &p2p](){
        crow::json::wvalue x;
        x["latest_id"] = (uint64_t)SSE::latestId();
        x["height"]    = (int)bc.getChain().size();
        x["mining"]    = is_mining.load();
        x["peers"]     = (int)p2p.peers.size();
        return x;
    });

    CROW_ROUTE(app, "/minerar_agora/<string>")
    ([&bc, &p2p](std::string external_addr){
        bool expected = false;
        if (!is_mining.compare_exchange_strong(expected, true)) {
            return crow::response(429, "O nó já está minerando um bloco no momento.");
        }
        std::thread t([&bc, &p2p, external_addr]() {
            is_mining.store(false); 
            async_mine(bc, p2p, external_addr);
        });
        t.detach();
        return crow::response(200, "Mineração iniciada para o endereço solicitado.");
    });

    CROW_ROUTE(app, "/api/balance/<string>")
    ([&bc](std::string addr){
        crow::json::wvalue x;
        x["address"] = addr;
        x["balance"] = bc.getBalance(addr);
        return x;
    });

    CROW_ROUTE(app, "/")
    ([]{
        std::ifstream file("index.html");
        if (!file.is_open()) return crow::response(404, "index.html not found");
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        crow::response res(200, content);
        res.set_header("Content-Type", "text/html; charset=utf-8");
        return res;
    });

    CROW_ROUTE(app, "/wallet/new")
    ([]{
        Wallet w;
        w.generateKey();
        crow::json::wvalue x;
        x["address"] = w.address;
        x["seed"] = w.seed;
        return x;
    });

    CROW_ROUTE(app, "/wallet/import").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto j = crow::json::load(req.body);
        if (!j) return crow::response(400, "JSON invalido");
        std::string seed = j["seed"].s();
        try {
            Wallet w;
            w.fromSeed(seed);
            crow::json::wvalue x;
            x["status"] = "success";
            x["address"] = w.address;
            return crow::response(x);
        } catch (...) {
            crow::json::wvalue x;
            x["status"] = "error";
            x["reason"] = "Seed invalida";
            return crow::response(400, x);
        }
    });

    CROW_ROUTE(app, "/balance/<string>")
    ([&bc](std::string addr){
        double bal = bc.getBalance(addr);
        crow::json::wvalue x;
        x["address"] = addr;
        x["balance_mz"] = bal;
        x["balance_mits"] = (long long)(bal * 100000000.0);
        return x;
    });

    CROW_ROUTE(app, "/send").methods(crow::HTTPMethod::POST)
    ([&bc](const crow::request& req){
        auto j = crow::json::load(req.body);
        if (!j) { crow::json::wvalue e; e["reason"] = "JSON invalido"; return crow::response(400, e); }

        // Validate required fields exist and are the right type
        if (!j.has("from") || !j.has("to") || !j.has("amount")) {
            crow::json::wvalue e; e["reason"] = "Campos obrigatorios: from, to, amount"; return crow::response(400, e);
        }

        std::string seed;
        try {
            // seed can be missing or null when session expired
            if (j.has("seed") && j["seed"].t() == crow::json::type::String) {
                seed = (std::string)j["seed"].s();
            }
        } catch (...) { seed = ""; }

        if (seed.empty()) {
            crow::json::wvalue e;
            e["reason"] = "Sessao expirada. Desbloqueie sua carteira novamente.";
            e["code"] = "SESSION_EXPIRED";
            return crow::response(401, e);
        }

        std::string from, to;
        double amount = 0.0;
        try {
            from   = (std::string)j["from"].s();
            to     = (std::string)j["to"].s();
            // amount may be int or double in JSON
            if (j["amount"].t() == crow::json::type::Number)
                amount = j["amount"].d();
            else
                amount = std::stod((std::string)j["amount"].s());
        } catch (...) {
            crow::json::wvalue e; e["reason"] = "Dados invalidos (from/to/amount)"; return crow::response(400, e);
        }

        if (from.empty() || to.empty() || amount <= 0) {
            crow::json::wvalue e; e["reason"] = "Endereco ou valor invalido"; return crow::response(400, e);
        }

        try {
            bc.send(from, to, amount, seed);
            crow::json::wvalue res;
            res["status"] = "success";
            return crow::response(res);
        } catch (const std::exception& e) {
            crow::json::wvalue res;
            res["reason"] = std::string(e.what());
            return crow::response(400, res);
        }
    });

    CROW_ROUTE(app, "/chain")
    ([&bc]{
        crow::json::wvalue x;
        auto chain = bc.getChain();
        // Return last 100 blocks in reverse (newest first) for performance
        int start = std::max(0, (int)chain.size() - 100);
        crow::json::wvalue::list blocks;
        for (int i = (int)chain.size() - 1; i >= start; i--) {
            const auto& b = chain[i];
            crow::json::wvalue bv;
            bv["index"] = b.index;
            bv["hash"] = b.hash;
            bv["prevHash"] = b.prevHash;
            bv["timestamp"] = (long long)b.timestamp;
            bv["miner"] = b.minerAddress;
            bv["tx_count"] = (int)b.transactions.size();

            crow::json::wvalue::list txlist;
            for (const auto& tx : b.transactions) {
                crow::json::wvalue tv;
                tv["id"] = tx.id.empty() ? tx.hash : tx.id;
                bool isCoinbase = (tx.signature == "coinbase");
                tv["type"] = isCoinbase ? "coinbase" : "transfer";
                if (isCoinbase) {
                    // vout[0] = miner reward
                    if (!tx.vout.empty()) {
                        tv["to"] = tx.vout[0].address;
                        tv["amount"] = tx.vout[0].amount;
                    }
                    tv["fee"] = 0.0;
                } else {
                    // vout[0] = receiver, vout[1] = sender debit (negative)
                    if (tx.vout.size() >= 2) {
                        tv["to"]   = tx.vout[0].address;
                        tv["from"] = tx.vout[1].address;
                        double sent  = tx.vout[0].amount;
                        double debit = std::abs(tx.vout[1].amount);
                        tv["amount"] = sent;
                        tv["fee"]    = debit - sent;
                    } else if (!tx.vout.empty()) {
                        tv["to"]     = tx.vout[0].address;
                        tv["amount"] = tx.vout[0].amount;
                        tv["fee"]    = 0.0;
                    }
                }
                txlist.push_back(std::move(tv));
            }
            bv["transactions"] = std::move(txlist);
            blocks.push_back(std::move(bv));
        }
        x["blocks"] = std::move(blocks);
        x["height"] = (int)chain.size();
        return x;
    });

    CROW_ROUTE(app, "/block/<int>")
    ([&bc](int idx){
        auto chain = bc.getChain();
        if (idx < 0 || idx >= (int)chain.size()) {
            crow::json::wvalue err;
            err["error"] = "Bloco nao encontrado";
            return crow::response(404, err.dump());
        }
        const auto& b = chain[idx];
        crow::json::wvalue bv;
        bv["index"]     = b.index;
        bv["hash"]      = b.hash;
        bv["prevHash"]  = b.prevHash;
        bv["timestamp"] = (long long)b.timestamp;
        bv["miner"]     = b.minerAddress;
        bv["tx_count"]  = (int)b.transactions.size();
        crow::json::wvalue::list txlist;
        for (const auto& tx : b.transactions) {
            crow::json::wvalue tv;
            tv["id"]   = tx.id.empty() ? tx.hash : tx.id;
            bool isCoinbase = (tx.signature == "coinbase");
            tv["type"] = isCoinbase ? "coinbase" : "transfer";
            if (isCoinbase) {
                if (!tx.vout.empty()) { tv["to"] = tx.vout[0].address; tv["amount"] = tx.vout[0].amount; }
                tv["fee"] = 0.0;
            } else if (tx.vout.size() >= 2) {
                tv["to"]     = tx.vout[0].address;
                tv["from"]   = tx.vout[1].address;
                double sent  = tx.vout[0].amount;
                double debit = std::abs(tx.vout[1].amount);
                tv["amount"] = sent;
                tv["fee"]    = debit - sent;
            } else if (!tx.vout.empty()) {
                tv["to"] = tx.vout[0].address; tv["amount"] = tx.vout[0].amount; tv["fee"] = 0.0;
            }
            txlist.push_back(std::move(tv));
        }
        bv["transactions"] = std::move(txlist);
        return crow::response(bv.dump());
    });

    // Busca bloco por hash — usado pelo explorer do frontend
    CROW_ROUTE(app, "/block/hash/<string>")
    ([&bc](std::string hash){
        auto chain = bc.getChain();
        for (const auto& b : chain) {
            if (b.hash == hash) {
                crow::json::wvalue bv;
                bv["index"]     = b.index;
                bv["hash"]      = b.hash;
                bv["prevHash"]  = b.prevHash;
                bv["timestamp"] = (long long)b.timestamp;
                bv["miner"]     = b.minerAddress;
                bv["tx_count"]  = (int)b.transactions.size();
                crow::json::wvalue::list txlist;
                for (const auto& tx : b.transactions) {
                    crow::json::wvalue tv;
                    tv["id"]   = tx.id.empty() ? tx.hash : tx.id;
                    bool isCoinbase = (tx.signature == "coinbase");
                    tv["type"] = isCoinbase ? "coinbase" : "transfer";
                    if (isCoinbase) {
                        if (!tx.vout.empty()) { tv["to"] = tx.vout[0].address; tv["amount"] = tx.vout[0].amount; }
                        tv["fee"] = 0.0;
                    } else if (tx.vout.size() >= 2) {
                        tv["to"]     = tx.vout[0].address;
                        tv["from"]   = tx.vout[1].address;
                        double sent  = tx.vout[0].amount;
                        double debit = std::abs(tx.vout[1].amount);
                        tv["amount"] = sent;
                        tv["fee"]    = debit - sent;
                    } else if (!tx.vout.empty()) {
                        tv["to"] = tx.vout[0].address; tv["amount"] = tx.vout[0].amount; tv["fee"] = 0.0;
                    }
                    txlist.push_back(std::move(tv));
                }
                bv["transactions"] = std::move(txlist);
                return crow::response(bv.dump());
            }
        }
        crow::json::wvalue err;
        err["error"] = "Hash nao encontrado";
        return crow::response(404, err.dump());
    });

    CROW_ROUTE(app, "/mempool")
    ([&bc]{
        crow::json::wvalue x;
        auto pending = Storage::loadMempool("data/mempool.dat");
        crow::json::wvalue::list txlist;
        for (const auto& tx : pending) {
            crow::json::wvalue tv;
            tv["id"] = tx.id;
            bool isCoinbase = (tx.signature == "coinbase");
            tv["type"] = isCoinbase ? "coinbase" : "transfer";
            if (!isCoinbase && tx.vout.size() >= 2) {
                tv["to"]     = tx.vout[0].address;
                tv["from"]   = tx.vout[1].address;
                tv["amount"] = tx.vout[0].amount;
                tv["fee"]    = std::abs(tx.vout[1].amount) - tx.vout[0].amount;
            } else if (!tx.vout.empty()) {
                tv["to"]     = tx.vout[0].address;
                tv["amount"] = tx.vout[0].amount;
                tv["fee"]    = 0.0;
            }
            txlist.push_back(std::move(tv));
        }
        x["pending"] = std::move(txlist);
        x["count"] = (int)pending.size();
        return x;
    });

    CROW_ROUTE(app, "/status")
    ([&bc, &p2p]{
        auto net_cfg = NetworkConfig::getConfig();
        crow::json::wvalue x;
        x["protocol"]["difficulty"]     = bc.getDifficulty();
        x["protocol"]["total_supply"]   = bc.getTotalSupply();
        x["protocol"]["max_supply"]     = bc.getMaxSupply();
        x["chain"]["height"]            = bc.getHeight();
        x["chain"]["chain_work"]        = (long long)bc.getChainWork();
        x["network"]["peers_count"]     = (int)p2p.peers.size();
        x["network"]["name"]            = net_cfg.name;
        x["network"]["magic"]           = net_cfg.magic_bytes;
        x["node"]["version"]            = net_cfg.protocol_version;
        x["node"]["max_block_size"]     = MAX_BLOCK_SIZE;
        x["node"]["max_block_txs"]      = MAX_BLOCK_TXS;
        return x;
    });

    // ─── GET /version — Handshake de identificação de rede ───────────────────
    CROW_ROUTE(app, "/version")
    ([&bc]{
        auto net_cfg = NetworkConfig::getConfig();
        std::string genesis_hash = "";
        auto ch = bc.getChain();
        if (!ch.empty()) genesis_hash = ch[0].hash;

        crow::json::wvalue x;
        x["version"]      = net_cfg.protocol_version;
        x["network"]      = net_cfg.name;
        x["genesis_hash"] = genesis_hash;
        x["height"]       = bc.getHeight();
        x["magic"]        = net_cfg.magic_bytes;
        x["services"]     = "FULL_NODE,MINER";
        return x;
    });

    // ─── P2P: troca de listas de peers (peer exchange) ────────────────────────
    CROW_ROUTE(app, "/peers/exchange").methods(crow::HTTPMethod::POST)
    ([&p2p](const crow::request& req){
        auto j = crow::json::load(req.body);
        if (!j) return crow::response(400, "JSON invalido");

        // Adiciona peers que o remetente conhece
        int added = 0;
        if (j.has("peers")) {
            for (const auto& pv : j["peers"]) {
                std::string purl = (std::string)pv.s();
                if (!purl.empty() && p2p.peers.find(purl) == p2p.peers.end()) {
                    p2p.add_peer(purl);
                    added++;
                }
            }
            if (added > 0) p2p.save_peers("data/peers.dat");
        }

        // Retorna nossa lista de peers
        crow::json::wvalue x;
        crow::json::wvalue::list list;
        for (const auto& peer : p2p.peers) list.push_back(peer);
        x["peers"] = std::move(list);
        x["added"] = added;
        return crow::response(x.dump());
    });

    // ─── P2P: list peers ──────────────────────────────────────────────────────
    CROW_ROUTE(app, "/peers")
    ([&p2p]{
        crow::json::wvalue x;
        crow::json::wvalue::list list;
        for (const auto& peer : p2p.peers) {
            crow::json::wvalue pv;
            pv["url"] = peer;
            list.push_back(std::move(pv));
        }
        x["peers"] = std::move(list);
        x["count"] = (int)p2p.peers.size();
        return x;
    });

    // ─── P2P: add peer ────────────────────────────────────────────────────────
    CROW_ROUTE(app, "/peers/add").methods(crow::HTTPMethod::POST)
    ([&p2p](const crow::request& req){
        auto j = crow::json::load(req.body);
        if (!j || !j.has("url")) return crow::response(400, "Missing 'url'");
        std::string url = (std::string)j["url"].s();
        if (url.empty()) return crow::response(400, "Empty URL");
        p2p.add_peer(url);
        p2p.save_peers("data/peers.dat");
        crow::json::wvalue x;
        x["status"] = "ok";
        x["peers_count"] = (int)p2p.peers.size();
        return crow::response(x.dump());
    });

    // ─── P2P: remove peer ─────────────────────────────────────────────────────
    CROW_ROUTE(app, "/peers/remove").methods(crow::HTTPMethod::POST)
    ([&p2p](const crow::request& req){
        auto j = crow::json::load(req.body);
        if (!j || !j.has("url")) return crow::response(400, "Missing 'url'");
        p2p.remove_peer((std::string)j["url"].s());
        p2p.save_peers("data/peers.dat");
        crow::json::wvalue x;
        x["status"] = "ok";
        x["peers_count"] = (int)p2p.peers.size();
        return crow::response(x.dump());
    });

    // ─── P2P: sync trigger ────────────────────────────────────────────────────
    CROW_ROUTE(app, "/peers/sync").methods(crow::HTTPMethod::POST)
    ([&bc, &p2p]{
        std::thread([&bc, &p2p](){
            p2p.sync_with_peers(bc, DB_PATH);
        }).detach();
        crow::json::wvalue x;
        x["status"] = "sync_started";
        return crow::response(x.dump());
    });

    // ─── P2P: serve blockchain for sync ──────────────────────────────────────
    // GET /blockchain?from=0&limit=500
    CROW_ROUTE(app, "/blockchain")
    ([&bc](const crow::request& req){
        int from  = 0;
        int limit = 500;
        auto from_p  = req.url_params.get("from");
        auto limit_p = req.url_params.get("limit");
        if (from_p)  try { from  = std::stoi(from_p);  } catch (...) {}
        if (limit_p) try { limit = std::stoi(limit_p); } catch (...) {}
        if (limit > 500) limit = 500;
        if (from  < 0)   from  = 0;

        const auto& chain = bc.getChain();

        std::string out = "{\"blocks\":[";
        bool first = true;
        for (int i = from; i < (int)chain.size() && i < from + limit; i++) {
            if (!first) out += ",";
            first = false;
            out += block_to_json_str(chain[i]);
        }
        out += "]}";

        crow::response res(out);
        res.set_header("Content-Type", "application/json");
        return res;
    });

    // ─── P2P: receive block from peer ─────────────────────────────────────────
    CROW_ROUTE(app, "/block").methods(crow::HTTPMethod::POST)
    ([&bc, &p2p](const crow::request& req){
        auto j = crow::json::load(req.body);
        if (!j) return crow::response(400, "Invalid JSON");

        Block b({}, {}, {}, {}, 0);
        if (!block_from_crow_json(j, b)) {
            return crow::response(400, "Failed to parse block");
        }

        const auto& chain = bc.getChain();
        int expected = (int)chain.size();

        if (b.index < expected) {
            crow::json::wvalue x; x["status"] = "already_have";
            return crow::response(200, x.dump());
        }
        if (b.index != expected) {
            crow::json::wvalue x; x["status"] = "index_mismatch";
            x["expected"] = expected; x["got"] = b.index;
            return crow::response(409, x.dump());
        }
        if (b.prevHash != chain.back().hash) {
            return crow::response(409, "prevHash mismatch");
        }

        // Verify PoW: hash must match computed value
        std::string actual = Blockchain::calculateProofOfWork(b.toHashString());
        if (actual != b.hash) {
            return crow::response(400, "Invalid hash");
        }

        // Accept the block
        bc.addBlock(b);
        Storage::saveBlockToDisk(b, DB_PATH);
        bc.interruptMining.store(true);

        // Relay to other peers (they will deduplicate via already_have)
        std::string block_json = req.body;
        for (const auto& peer : p2p.peers) {
            std::thread([peer, block_json](){
                CURL* curl = curl_easy_init();
                if (!curl) return;
                std::string url = peer + "/block";
                struct curl_slist* hdrs = curl_slist_append(nullptr, "Content-Type: application/json");
                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, block_json.c_str());
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
                curl_easy_perform(curl);
                curl_slist_free_all(hdrs);
                curl_easy_cleanup(curl);
            }).detach();
        }

        crow::json::wvalue x;
        x["status"] = "accepted";
        x["height"] = (int)bc.getChain().size();
        return crow::response(x.dump());
    });

    // ─── P2P: receive transaction relay from peer ────────────────────────────
    CROW_ROUTE(app, "/tx").methods(crow::HTTPMethod::POST)
    ([&bc](const crow::request& req){
        auto j = crow::json::load(req.body);
        if (!j) return crow::response(400, "Invalid JSON");
        try {
            Transaction t;
            t.id        = (std::string)j["id"].s();
            t.signature = (std::string)j["sig"].s();
            t.publicKey = (std::string)j["pub"].s();
            for (const auto& jv : j["vout"]) {
                TxOut o;
                o.address = (std::string)jv["addr"].s();
                o.amount  = jv["amt"].d();
                t.vout.push_back(o);
            }
            if (bc.verifyTransaction(t)) {
                Storage::saveMempool(t, "data/mempool.dat");
                crow::json::wvalue x; x["status"] = "accepted";
                return crow::response(x.dump());
            }
            return crow::response(400, "Transaction verification failed");
        } catch (...) {
            return crow::response(400, "Bad transaction data");
        }
    });

    // ─── HD Wallet: derive address from seed + index ─────────────────────────
    // GET /wallet/hd/<seed>/<index>
    // Retorna o endereço HD no caminho m/44'/1611'/0'/0/<index>
    CROW_ROUTE(app, "/wallet/hd/<string>/<int>")
    ([](std::string seed, int index){
        if (seed.empty() || seed.find(' ') == std::string::npos) {
            crow::json::wvalue e;
            e["error"] = "Seed deve ser uma frase de 12 palavras separadas por espaço.";
            return crow::response(400, e.dump());
        }
        if (index < 0 || index > 1000000) {
            crow::json::wvalue e;
            e["error"] = "Index deve ser entre 0 e 1.000.000";
            return crow::response(400, e.dump());
        }
        try {
            auto key = HDWallet::deriveAddress(seed, (uint32_t)index);
            crow::json::wvalue x;
            x["address"]    = key.address;
            x["path"]       = key.path;
            x["index"]      = index;
            x["public_key"] = key.publicKey;
            return crow::response(x.dump());
        } catch (const std::exception& e) {
            crow::json::wvalue err;
            err["error"] = std::string(e.what());
            return crow::response(500, err.dump());
        }
    });

    // POST /wallet/hd/batch — gera múltiplos endereços de uma vez
    CROW_ROUTE(app, "/wallet/hd/batch").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto j = crow::json::load(req.body);
        if (!j || !j.has("seed")) return crow::response(400, "Campo 'seed' obrigatorio");
        std::string seed = (std::string)j["seed"].s();
        int count = j.has("count") ? (int)j["count"].i() : 20;
        if (count < 1 || count > 100) count = 20;
        try {
            auto keys = HDWallet::generateAddresses(seed, count);
            crow::json::wvalue x;
            crow::json::wvalue::list list;
            for (const auto& k : keys) {
                crow::json::wvalue entry;
                entry["address"] = k.address;
                entry["path"]    = k.path;
                entry["index"]   = (int)k.index;
                list.push_back(std::move(entry));
            }
            x["addresses"] = std::move(list);
            x["count"]     = count;
            return crow::response(x.dump());
        } catch (const std::exception& e) {
            crow::json::wvalue err;
            err["error"] = std::string(e.what());
            return crow::response(500, err.dump());
        }
    });

    // ─── Multi-Sig: criar carteira M-de-N ────────────────────────────────────
    // POST /multisig/create
    // Body: { "signers": ["MZ...", "MZ..."], "required": 2 }
    CROW_ROUTE(app, "/multisig/create").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto j = crow::json::load(req.body);
        if (!j || !j.has("signers") || !j.has("required")) {
            return crow::response(400, "Campos obrigatorios: signers (array), required (int)");
        }
        std::vector<std::string> signers;
        for (const auto& sv : j["signers"]) signers.push_back((std::string)sv.s());
        int required = (int)j["required"].i();

        try {
            auto wallet = Multisig::createMultisigWallet(signers, required);
            crow::json::wvalue x;
            x["address"]  = wallet.address;
            x["required"] = wallet.required;
            x["total"]    = wallet.total;
            crow::json::wvalue::list sl;
            for (const auto& s : wallet.signers) sl.push_back(s);
            x["signers"]  = std::move(sl);
            x["type"]     = std::to_string(required) + "-de-" + std::to_string((int)signers.size());
            return crow::response(x.dump());
        } catch (const std::exception& e) {
            crow::json::wvalue err;
            err["error"] = std::string(e.what());
            return crow::response(400, err.dump());
        }
    });

    // POST /multisig/propose — cria transação multisig aguardando assinaturas
    // Body: { "from": "MZms...", "to": "MZ...", "amount": 1.0, "required": 2 }
    CROW_ROUTE(app, "/multisig/propose").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto j = crow::json::load(req.body);
        if (!j || !j.has("from") || !j.has("to") || !j.has("amount") || !j.has("required")) {
            return crow::response(400, "Campos: from, to, amount, required");
        }
        std::string from   = (std::string)j["from"].s();
        std::string to     = (std::string)j["to"].s();
        double amount      = j["amount"].d();
        int required       = (int)j["required"].i();

        Multisig::PendingMultisig p;
        p.txid      = Crypto::sha256_util(from + to + std::to_string(amount) + std::to_string(std::time(nullptr)));
        p.from      = from;
        p.to        = to;
        p.amount    = amount;
        p.required  = required;
        p.timestamp = (long)std::time(nullptr);

        Multisig::savePending(p, "data/multisig_pending.dat");

        crow::json::wvalue x;
        x["txid"]     = p.txid;
        x["status"]   = "pending";
        x["required"] = required;
        x["message"]  = "Transação criada. Aguardando " + std::to_string(required) + " assinatura(s).";
        return crow::response(x.dump());
    });

    // POST /multisig/sign — adiciona uma assinatura parcial
    // Body: { "txid": "...", "signer": "MZ...", "seed": "12 palavras..." }
    CROW_ROUTE(app, "/multisig/sign").methods(crow::HTTPMethod::POST)
    ([&bc](const crow::request& req){
        auto j = crow::json::load(req.body);
        if (!j || !j.has("txid") || !j.has("signer") || !j.has("seed")) {
            return crow::response(400, "Campos: txid, signer, seed");
        }
        std::string txid   = (std::string)j["txid"].s();
        std::string signer = (std::string)j["signer"].s();
        std::string seed   = (std::string)j["seed"].s();

        // Gera a assinatura com a seed do signatário
        std::string signature = Crypto::sha256_util(txid + signer + seed + "MAZE_MULTISIG_V1");

        try {
            bool ready = Multisig::addSignature(txid, signer, signature, "data/multisig_pending.dat");
            crow::json::wvalue x;
            x["txid"]      = txid;
            x["signer"]    = signer;
            x["signature"] = signature.substr(0, 16) + "...";
            x["ready"]     = ready;
            x["status"]    = ready ? "complete — pronto para broadcast" : "aguardando mais assinaturas";
            return crow::response(x.dump());
        } catch (const std::exception& e) {
            crow::json::wvalue err;
            err["error"] = std::string(e.what());
            return crow::response(400, err.dump());
        }
    });

    // GET /multisig/pending — lista transações aguardando assinaturas
    CROW_ROUTE(app, "/multisig/pending")
    ([]{
        auto all = Multisig::loadPending("data/multisig_pending.dat");
        crow::json::wvalue x;
        crow::json::wvalue::list list;
        for (const auto& p : all) {
            crow::json::wvalue pv;
            pv["txid"]            = p.txid;
            pv["from"]            = p.from;
            pv["to"]              = p.to;
            pv["amount"]          = p.amount;
            pv["required"]        = p.required;
            pv["sigs_collected"]  = (int)p.collected_sigs.size();
            pv["ready"]           = ((int)p.collected_sigs.size() >= p.required);
            pv["timestamp"]       = (long long)p.timestamp;
            crow::json::wvalue::list signed_by;
            for (const auto& s : p.signed_by) signed_by.push_back(s);
            pv["signed_by"]       = std::move(signed_by);
            list.push_back(std::move(pv));
        }
        x["pending"] = std::move(list);
        x["count"]   = (int)all.size();
        return x;
    });

    // POST /multisig/broadcast — transmite tx multisig com M assinaturas coletadas
    // Body: { "txid": "..." }
    CROW_ROUTE(app, "/multisig/broadcast").methods(crow::HTTPMethod::POST)
    ([&bc](const crow::request& req){
        auto j = crow::json::load(req.body);
        if (!j || !j.has("txid")) return crow::response(400, "Campo 'txid' obrigatorio");
        std::string txid = (std::string)j["txid"].s();

        auto all = Multisig::loadPending("data/multisig_pending.dat");
        for (const auto& p : all) {
            if (p.txid != txid) continue;

            if ((int)p.collected_sigs.size() < p.required) {
                crow::json::wvalue e;
                e["error"] = "Assinaturas insuficientes: " + std::to_string(p.collected_sigs.size())
                           + "/" + std::to_string(p.required);
                return crow::response(400, e.dump());
            }

            // Constrói a transação final e coloca no mempool
            Transaction tx;
            tx.id        = p.txid;
            tx.signature = p.collected_sigs[0]; // assinatura primária
            tx.publicKey = "MAZE_MULTISIG_" + std::to_string(p.required) + "of" + std::to_string((int)p.collected_sigs.size());
            tx.vout.push_back({p.to,   p.amount});
            tx.vout.push_back({p.from, -(p.amount * 1.01)}); // 1% fee
            tx.timestamp = (long)std::time(nullptr);

            Storage::saveMempool(tx, "data/mempool.dat");
            Multisig::removePending(txid, "data/multisig_pending.dat");

            crow::json::wvalue x;
            x["status"] = "broadcast";
            x["txid"]   = txid;
            x["message"] = "Transação multisig enviada ao mempool com sucesso!";
            return crow::response(x.dump());
        }

        crow::json::wvalue e;
        e["error"] = "Transação não encontrada nas pendências.";
        return crow::response(404, e.dump());
    });

    // ─── Mempool: purgar transações expiradas ─────────────────────────────────
    CROW_ROUTE(app, "/mempool/purge").methods(crow::HTTPMethod::POST)
    ([]{
        int removed = MempoolExpiry::purgeExpired("data/mempool.dat");
        crow::json::wvalue x;
        x["status"]  = "ok";
        x["removed"] = removed;
        x["message"] = std::to_string(removed) + " transações expiradas removidas do mempool.";
        return crow::response(x.dump());
    });

    // ─── Testnet info ─────────────────────────────────────────────────────────
    CROW_ROUTE(app, "/network")
    ([&bc, &p2p]{
        auto cfg = NetworkConfig::getConfig();
        crow::json::wvalue x;
        x["name"]            = cfg.name;
        x["prefix"]          = cfg.address_prefix;
        x["magic"]           = cfg.magic_bytes;
        x["version"]         = cfg.protocol_version;
        x["min_difficulty"]  = cfg.min_difficulty;
        x["is_testnet"]      = NetworkConfig::isTestnet();
        x["height"]          = bc.getHeight();
        x["peers_count"]     = (int)p2p.peers.size();
        // Return peer URLs for the frontend peer list
        std::vector<std::string> peer_list(p2p.peers.begin(), p2p.peers.end());
        crow::json::wvalue arr = crow::json::wvalue::list();
        for (int i = 0; i < (int)peer_list.size(); i++) arr[i] = peer_list[i];
        x["peers"] = std::move(arr);
        return x;
    });

    // ─── DB Integrity ─────────────────────────────────────────────────────────
    // GET /db/integrity — verifica se o arquivo de blockchain foi alterado fora do protocolo
    CROW_ROUTE(app, "/db/integrity")
    ([]{
        const std::string _db   = "data/blocks/blk00000.dat";
        const std::string _hash = "data/blockchain.hash";
        bool ok = DBIntegrity::VerifyIntegrity(_db, _hash);
        crow::json::wvalue x;
        x["status"]    = ok ? "ok" : "corrupted";
        x["ok"]        = ok;
        x["data_file"] = _db;
        x["message"]   = ok ? "Arquivo de blockchain integro." : "ALERTA: Arquivo modificado fora do protocolo!";
        return crow::response(x.dump());
    });

    // POST /db/integrity/update — recalcula e salva o hash de integridade
    CROW_ROUTE(app, "/db/integrity/update").methods(crow::HTTPMethod::POST)
    ([]{
        const std::string _db   = "data/blocks/blk00000.dat";
        const std::string _hash = "data/blockchain.hash";
        DBIntegrity::UpdateHash(_db, _hash);
        crow::json::wvalue x;
        x["status"]  = "updated";
        x["message"] = "Hash de integridade atualizado com sucesso.";
        return crow::response(x.dump());
    });

    // ─── External Miner: block template ──────────────────────────────────────
    // GET /getblocktemplate/<address>
    // Returns everything a remote miner needs to mine a block and submit it via POST /block
    CROW_ROUTE(app, "/getblocktemplate/<string>")
    ([&bc](std::string miner_addr){
        if (miner_addr.length() < 30 || miner_addr.substr(0, 2) != "MZ") {
            crow::json::wvalue err;
            err["error"] = "Endereço inválido. Use um endereço MZ válido.";
            return crow::response(400, err.dump());
        }

        bc.adjustDifficulty();
        int  next_index = bc.getHeight();
        std::string prev_hash = bc.getLastBlock().hash;
        int  diff    = bc.getDifficulty();
        double reward = bc.getBlockReward(next_index);

        double remaining = 20000000.0 - bc.getTotalSupply();
        if (reward > remaining) reward = (remaining > 0 ? remaining : 0);

        long long now = (long long)std::time(nullptr);
        std::string coinbase_id = "coinbase_h" + std::to_string(next_index) + "_" + std::to_string(now);

        crow::json::wvalue tmpl;
        tmpl["index"]      = next_index;
        tmpl["prevHash"]   = prev_hash;
        tmpl["difficulty"] = diff;
        tmpl["target"]     = std::string(diff, '0');
        tmpl["reward"]     = reward;
        tmpl["timestamp"]  = now;
        tmpl["miner"]      = miner_addr;

        crow::json::wvalue cb;
        cb["id"]  = coinbase_id;
        cb["sig"] = "coinbase";
        cb["pub"] = "MAZE_BITCOIN_CORE_ENGINE_V1";
        crow::json::wvalue::list vout_list;
        crow::json::wvalue vout0;
        vout0["addr"] = miner_addr;
        vout0["amt"]  = reward;
        vout_list.push_back(std::move(vout0));
        cb["vout"] = std::move(vout_list);

        tmpl["coinbase"] = std::move(cb);

        return crow::response(tmpl.dump());
    });

    // ═══════════════════════════════════════════════════════════════════════════
    // NOVOS ENDPOINTS — ~150 Features
    // ═══════════════════════════════════════════════════════════════════════════

    // ── Metrics (Prometheus-compatible) ────────────────────────────────────────
    // GET /metrics — Prometheus text format
    CROW_ROUTE(app, "/metrics")
    ([&bc]{
        Metrics::incApiRequest();
        Metrics::setSupply(bc.getTotalSupply());
        Metrics::setDifficulty(bc.getDifficulty());
        Metrics::blocks_mined.store(bc.getHeight());
        auto res = crow::response(200, Metrics::prometheus());
        res.set_header("Content-Type", "text/plain; version=0.0.4; charset=utf-8");
        return res;
    });

    // GET /metrics/json — JSON format for dashboard
    CROW_ROUTE(app, "/metrics/json")
    ([&bc]{
        Metrics::incApiRequest();
        Metrics::setSupply(bc.getTotalSupply());
        Metrics::setDifficulty(bc.getDifficulty());
        Metrics::blocks_mined.store(bc.getHeight());
        auto res = crow::response(200, Metrics::toJSON());
        res.set_header("Content-Type", "application/json");
        return res;
    });

    // ── Script VM ──────────────────────────────────────────────────────────────
    // POST /script/decode — decode a hex script to ASM + type
    CROW_ROUTE(app, "/script/decode").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        Metrics::incApiRequest();
        auto j = crow::json::load(req.body);
        if (!j || !j.has("hex")) {
            crow::json::wvalue e; e["error"] = "Campo 'hex' obrigatório";
            return crow::response(400, e.dump());
        }
        std::string hexScript = (std::string)j["hex"].s();
        try {
            Script::ByteVec script = Script::fromHex(hexScript);
            Script::ScriptType t   = Script::classify(script);
            std::string asm_str    = Script::decode(hexScript);
            crow::json::wvalue x;
            x["hex"]  = hexScript;
            x["type"] = Script::scriptTypeName(t);
            x["asm"]  = asm_str;
            x["size"] = (int)script.size();
            return crow::response(x.dump());
        } catch (const std::exception& e) {
            crow::json::wvalue err; err["error"] = std::string(e.what());
            return crow::response(400, err.dump());
        }
    });

    // POST /script/eval — evaluate scriptSig + scriptPubKey
    CROW_ROUTE(app, "/script/eval").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        Metrics::incApiRequest();
        auto j = crow::json::load(req.body);
        if (!j || !j.has("scriptsig") || !j.has("scriptpubkey")) {
            crow::json::wvalue e; e["error"] = "Campos: scriptsig, scriptpubkey";
            return crow::response(400, e.dump());
        }
        try {
            auto sig = Script::fromHex((std::string)j["scriptsig"].s());
            auto pub = Script::fromHex((std::string)j["scriptpubkey"].s());
            bool doTrace = j.has("trace") && j["trace"].b();
            auto result  = Script::evaluate(sig, pub, "", doTrace);
            crow::json::wvalue x;
            x["success"] = result.success;
            x["error"]   = result.error;
            crow::json::wvalue::list tl;
            for (const auto& t : result.trace) tl.push_back(t);
            x["trace"] = std::move(tl);
            return crow::response(x.dump());
        } catch (const std::exception& e) {
            crow::json::wvalue err; err["error"] = std::string(e.what());
            return crow::response(400, err.dump());
        }
    });

    // POST /script/build — build a standard script
    CROW_ROUTE(app, "/script/build").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        Metrics::incApiRequest();
        auto j = crow::json::load(req.body);
        if (!j || !j.has("type")) {
            crow::json::wvalue e; e["error"] = "Campo 'type' obrigatório (p2pkh|p2sh|p2wpkh|p2wsh|p2tr|op_return)";
            return crow::response(400, e.dump());
        }
        std::string type = (std::string)j["type"].s();
        std::string data = j.has("data") ? (std::string)j["data"].s() : "";
        try {
            Script::ByteVec script;
            if (type == "p2pkh" || type == "P2PKH") {
                script = Script::buildP2PKH(Script::fromHex(data));
            } else if (type == "p2sh" || type == "P2SH") {
                script = Script::buildP2SH(Script::fromHex(data));
            } else if (type == "p2wpkh" || type == "P2WPKH") {
                script = Script::buildP2WPKH(Script::fromHex(data));
            } else if (type == "p2wsh" || type == "P2WSH") {
                script = Script::buildP2WSH(Script::fromHex(data));
            } else if (type == "p2tr" || type == "P2TR") {
                script = Script::buildP2TR(Script::fromHex(data));
            } else if (type == "op_return" || type == "OP_RETURN") {
                script = Script::buildOpReturn(Script::fromHex(data));
            } else {
                crow::json::wvalue e; e["error"] = "Tipo desconhecido: " + type;
                return crow::response(400, e.dump());
            }
            crow::json::wvalue x;
            x["hex"]  = Script::toHex(script);
            x["type"] = type;
            x["size"] = (int)script.size();
            return crow::response(x.dump());
        } catch (const std::exception& e) {
            crow::json::wvalue err; err["error"] = std::string(e.what());
            return crow::response(400, err.dump());
        }
    });

    // ── Bech32 Encoding ────────────────────────────────────────────────────────
    // GET /bech32/encode?type=p2wpkh&data=<hex>&testnet=0
    CROW_ROUTE(app, "/bech32/encode")
    ([](const crow::request& req){
        Metrics::incApiRequest();
        std::string type    = req.url_params.get("type")    ? req.url_params.get("type")    : "p2wpkh";
        std::string data    = req.url_params.get("data")    ? req.url_params.get("data")    : "";
        bool testnet        = req.url_params.get("testnet") && std::string(req.url_params.get("testnet")) == "1";
        if (data.empty()) {
            crow::json::wvalue e; e["error"] = "Parâmetro 'data' obrigatório (hex)";
            return crow::response(400, e.dump());
        }
        try {
            auto bytes = Bech32::decode(data).witprog.size() > 0
                ? Bech32::decode(data).witprog
                : std::vector<uint8_t>();
            // data is raw hex
            auto raw = Script::fromHex(data);
            std::string addr;
            if (type == "p2wpkh") addr = Bech32::encodeP2WPKH(raw, testnet);
            else if (type == "p2wsh") addr = Bech32::encodeP2WSH(raw, testnet);
            else if (type == "p2tr")  addr = Bech32::encodeP2TR(raw, testnet);
            else addr = Bech32::encode(testnet ? "tmaze" : "maze", 0, raw);
            crow::json::wvalue x;
            x["address"]  = addr;
            x["type"]     = type;
            x["testnet"]  = testnet;
            x["valid"]    = !addr.empty();
            return crow::response(x.dump());
        } catch (const std::exception& e) {
            crow::json::wvalue err; err["error"] = std::string(e.what());
            return crow::response(400, err.dump());
        }
    });

    // GET /bech32/decode?addr=<bech32addr>
    CROW_ROUTE(app, "/bech32/decode")
    ([](const crow::request& req){
        Metrics::incApiRequest();
        std::string addr = req.url_params.get("addr") ? req.url_params.get("addr") : "";
        if (addr.empty()) {
            crow::json::wvalue e; e["error"] = "Parâmetro 'addr' obrigatório";
            return crow::response(400, e.dump());
        }
        auto result = Bech32::decode(addr);
        crow::json::wvalue x;
        x["valid"]   = result.valid;
        x["error"]   = result.error;
        x["witver"]  = result.witver;
        if (result.valid) {
            x["program"] = Script::toHex(result.witprog);
            x["length"]  = (int)result.witprog.size();
            x["encoding"]= (result.witver == 0) ? "bech32" : "bech32m";
        }
        return crow::response(x.dump());
    });

    // ── Fee Estimation ─────────────────────────────────────────────────────────
    // GET /fee/estimate?target=3&inputs=2&outputs=2&segwit=0
    CROW_ROUTE(app, "/fee/estimate")
    ([](const crow::request& req){
        Metrics::incApiRequest();
        int target  = req.url_params.get("target")  ? std::stoi(req.url_params.get("target"))  : 3;
        int inputs  = req.url_params.get("inputs")  ? std::stoi(req.url_params.get("inputs"))  : 1;
        int outputs = req.url_params.get("outputs") ? std::stoi(req.url_params.get("outputs")) : 2;
        bool segwit = req.url_params.get("segwit")  && std::string(req.url_params.get("segwit")) == "1";
        auto rates  = FeeEstimation::estimate(target);
        int  txSize = FeeEstimation::estimateTxSize(inputs, outputs, segwit);
        crow::json::wvalue x;
        x["target_blocks"]  = target;
        x["tx_size_bytes"]  = txSize;
        x["segwit"]         = segwit;
        x["fee_fast"]       = FeeEstimation::calcFee(txSize, rates.fast);
        x["fee_normal"]     = FeeEstimation::calcFee(txSize, rates.normal);
        x["fee_economy"]    = FeeEstimation::calcFee(txSize, rates.economy);
        x["fee_minimum"]    = FeeEstimation::calcFee(txSize, rates.minimum);
        x["rate_fast"]      = rates.fast;
        x["rate_normal"]    = rates.normal;
        x["rate_economy"]   = rates.economy;
        x["rate_minimum"]   = rates.minimum;
        return crow::response(x.dump());
    });

    // ── Merkle Proofs ──────────────────────────────────────────────────────────
    // GET /merkle/root?txids=hash1,hash2,...
    CROW_ROUTE(app, "/merkle/root")
    ([](const crow::request& req){
        Metrics::incApiRequest();
        std::string txidsParam = req.url_params.get("txids") ? req.url_params.get("txids") : "";
        if (txidsParam.empty()) {
            crow::json::wvalue e; e["error"] = "Parâmetro 'txids' obrigatório (comma-separated)";
            return crow::response(400, e.dump());
        }
        std::vector<std::string> txids;
        std::istringstream ss(txidsParam);
        std::string t;
        while (std::getline(ss, t, ',')) if (!t.empty()) txids.push_back(t);
        crow::json::wvalue x;
        x["merkle_root"] = Merkle::calcMerkleRoot(txids);
        x["tx_count"]    = (int)txids.size();
        return crow::response(x.dump());
    });

    // POST /merkle/proof — generate merkle proof for a tx
    CROW_ROUTE(app, "/merkle/proof").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        Metrics::incApiRequest();
        auto j = crow::json::load(req.body);
        if (!j || !j.has("txids") || !j.has("index")) {
            crow::json::wvalue e; e["error"] = "Campos: txids (array), index (int)";
            return crow::response(400, e.dump());
        }
        try {
            std::vector<std::string> txids;
            for (int i = 0; i < (int)j["txids"].size(); i++)
                txids.push_back((std::string)j["txids"][i].s());
            int idx = (int)j["index"].i();
            auto proof = Merkle::generateProof(txids, idx);
            crow::json::wvalue x;
            x["txid"]        = proof.txid;
            x["merkle_root"] = proof.merkleRoot;
            x["tx_index"]    = proof.txIndex;
            x["valid"]       = proof.isValid();
            crow::json::wvalue::list pathList;
            for (const auto& n : proof.path) {
                crow::json::wvalue pv;
                pv["hash"]    = n.hash;
                pv["position"]= n.isRight ? "right" : "left";
                pathList.push_back(std::move(pv));
            }
            x["proof_path"] = std::move(pathList);
            x["serialized"] = proof.serialize();
            return crow::response(x.dump());
        } catch (const std::exception& e) {
            crow::json::wvalue err; err["error"] = std::string(e.what());
            return crow::response(400, err.dump());
        }
    });

    // POST /merkle/verify — verify a merkle proof (SPV)
    CROW_ROUTE(app, "/merkle/verify").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        Metrics::incApiRequest();
        auto j = crow::json::load(req.body);
        if (!j || !j.has("txid") || !j.has("merkle_root") || !j.has("proof")) {
            crow::json::wvalue e; e["error"] = "Campos: txid, merkle_root, proof (array of {hash, position})";
            return crow::response(400, e.dump());
        }
        try {
            std::vector<Merkle::ProofNode> path;
            for (int i = 0; i < (int)j["proof"].size(); i++) {
                Merkle::ProofNode n;
                n.hash    = (std::string)j["proof"][i]["hash"].s();
                n.isRight = (std::string)j["proof"][i]["position"].s() == "right";
                path.push_back(n);
            }
            bool valid = Merkle::spvVerify(
                (std::string)j["txid"].s(),
                (std::string)j["merkle_root"].s(),
                path
            );
            crow::json::wvalue x;
            x["valid"] = valid;
            x["txid"]  = (std::string)j["txid"].s();
            return crow::response(x.dump());
        } catch (const std::exception& e) {
            crow::json::wvalue err; err["error"] = std::string(e.what());
            return crow::response(400, err.dump());
        }
    });

    // ── Schnorr Signatures ─────────────────────────────────────────────────────
    // POST /schnorr/sign — sign 32-byte message with private key
    CROW_ROUTE(app, "/schnorr/sign").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        Metrics::incApiRequest();
        auto j = crow::json::load(req.body);
        if (!j || !j.has("privkey") || !j.has("message")) {
            crow::json::wvalue e; e["error"] = "Campos: privkey (32-byte hex), message (32-byte hex)";
            return crow::response(400, e.dump());
        }
        try {
            auto priv = Schnorr::fromHex((std::string)j["privkey"].s());
            auto msg  = Schnorr::fromHex((std::string)j["message"].s());
            if (priv.size() != 32) throw std::invalid_argument("privkey deve ser 32 bytes (64 hex)");
            if (msg.size()  != 32) throw std::invalid_argument("message deve ser 32 bytes (64 hex) — use SHA256 do conteúdo");
            auto kp  = Schnorr::makeKeyPair(priv);
            auto sig = Schnorr::sign(priv, msg);
            crow::json::wvalue x;
            x["signature"] = Schnorr::toHex(sig);
            x["pubkey"]    = Schnorr::toHex(kp.pubkey);
            x["message"]   = Schnorr::toHex(msg);
            return crow::response(x.dump());
        } catch (const std::exception& e) {
            crow::json::wvalue err; err["error"] = std::string(e.what());
            return crow::response(400, err.dump());
        }
    });

    // POST /schnorr/verify — verify Schnorr signature
    CROW_ROUTE(app, "/schnorr/verify").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        Metrics::incApiRequest();
        auto j = crow::json::load(req.body);
        if (!j || !j.has("pubkey") || !j.has("message") || !j.has("signature")) {
            crow::json::wvalue e; e["error"] = "Campos: pubkey, message, signature (todos hex 32/32/64 bytes)";
            return crow::response(400, e.dump());
        }
        try {
            auto pub = Schnorr::fromHex((std::string)j["pubkey"].s());
            auto msg = Schnorr::fromHex((std::string)j["message"].s());
            auto sig = Schnorr::fromHex((std::string)j["signature"].s());
            bool ok  = Schnorr::verify(pub, msg, sig);
            crow::json::wvalue x;
            x["valid"]     = ok;
            x["pubkey"]    = Schnorr::toHex(pub);
            return crow::response(x.dump());
        } catch (const std::exception& e) {
            crow::json::wvalue err; err["error"] = std::string(e.what());
            return crow::response(400, err.dump());
        }
    });

    // POST /taproot/tweak — compute Taproot output key
    CROW_ROUTE(app, "/taproot/tweak").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        Metrics::incApiRequest();
        auto j = crow::json::load(req.body);
        if (!j || !j.has("internal_key")) {
            crow::json::wvalue e; e["error"] = "Campo: internal_key (32-byte hex x-only pubkey)";
            return crow::response(400, e.dump());
        }
        try {
            auto internalKey  = Schnorr::fromHex((std::string)j["internal_key"].s());
            std::string merkleHex = j.has("merkle_root") ? (std::string)j["merkle_root"].s() : std::string(64,'0');
            auto merkleRoot   = Schnorr::fromHex(merkleHex);
            auto outputKey    = Schnorr::taprootTweak(internalKey, merkleRoot);
            auto p2trAddr     = Bech32::encodeP2TR(outputKey, false);
            crow::json::wvalue x;
            x["internal_key"] = Schnorr::toHex(internalKey);
            x["merkle_root"]  = merkleHex;
            x["output_key"]   = Schnorr::toHex(outputKey);
            x["p2tr_address"] = p2trAddr;
            return crow::response(x.dump());
        } catch (const std::exception& e) {
            crow::json::wvalue err; err["error"] = std::string(e.what());
            return crow::response(400, err.dump());
        }
    });

    // ── Version Bits / Soft Forks ──────────────────────────────────────────────
    // GET /softforks — list all soft fork deployments and status
    CROW_ROUTE(app, "/softforks")
    ([]{
        Metrics::incApiRequest();
        auto res = crow::response(200, VersionBits::toJSON());
        res.set_header("Content-Type", "application/json");
        return res;
    });

    // GET /softforks/<name> — status of a specific deployment
    CROW_ROUTE(app, "/softforks/<string>")
    ([](std::string name){
        Metrics::incApiRequest();
        auto it = VersionBits::g_deployments.find(name);
        if (it == VersionBits::g_deployments.end()) {
            crow::json::wvalue e; e["error"] = "Deployment não encontrado: " + name;
            return crow::response(404, e.dump());
        }
        crow::json::wvalue x;
        x["name"]      = name;
        x["bit"]       = (int)it->second.bit;
        x["state"]     = VersionBits::stateName(it->second.state);
        x["active"]    = (it->second.state == VersionBits::State::ACTIVE);
        x["signaling"] = it->second.signalingCount;
        x["threshold"] = it->second.threshold;
        x["period"]    = it->second.period;
        return crow::response(x.dump());
    });

    // ── Orphan Pool ────────────────────────────────────────────────────────────
    // GET /orphans — orphan transactions and blocks
    CROW_ROUTE(app, "/orphans")
    ([]{
        Metrics::incApiRequest();
        crow::json::wvalue x;
        x["orphan_txs"]    = OrphanPool::orphanTxCount();
        x["orphan_blocks"] = OrphanPool::orphanBlockCount();
        return crow::response(x.dump());
    });

    // POST /orphans/purge — purge expired orphans
    CROW_ROUTE(app, "/orphans/purge").methods(crow::HTTPMethod::POST)
    ([]{
        Metrics::incApiRequest();
        int txRemoved  = OrphanPool::purgeExpiredTxs();
        int blkRemoved = OrphanPool::purgeExpiredBlocks();
        crow::json::wvalue x;
        x["txs_removed"]    = txRemoved;
        x["blocks_removed"] = blkRemoved;
        x["status"]         = "ok";
        return crow::response(x.dump());
    });

    // ── Ban Score / Peer Security ──────────────────────────────────────────────
    // GET /banned — list of banned peers
    CROW_ROUTE(app, "/banned")
    ([]{
        Metrics::incApiRequest();
        auto list = BanScore::getBannedList();
        crow::json::wvalue::list jlist;
        int64_t now = (int64_t)std::time(nullptr);
        for (const auto& kv : list) {
            crow::json::wvalue pv;
            pv["ip"]         = kv.first;
            pv["banned_until"]= kv.second;
            pv["remaining_s"] = kv.second - now;
            jlist.push_back(std::move(pv));
        }
        crow::json::wvalue x;
        x["banned"] = std::move(jlist);
        x["count"]  = (int)list.size();
        return crow::response(x.dump());
    });

    // POST /ban — manually ban a peer IP
    CROW_ROUTE(app, "/ban").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        Metrics::incApiRequest();
        auto j = crow::json::load(req.body);
        if (!j || !j.has("ip")) {
            crow::json::wvalue e; e["error"] = "Campo: ip"; return crow::response(400, e.dump());
        }
        std::string ip  = (std::string)j["ip"].s();
        int64_t dur     = j.has("duration") ? (int64_t)j["duration"].i() : BanScore::BAN_DURATION_SEC;
        BanScore::ban(ip, dur);
        crow::json::wvalue x;
        x["status"]   = "banned";
        x["ip"]       = ip;
        x["duration"] = dur;
        return crow::response(x.dump());
    });

    // POST /unban — remove ban from peer
    CROW_ROUTE(app, "/unban").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        Metrics::incApiRequest();
        auto j = crow::json::load(req.body);
        if (!j || !j.has("ip")) {
            crow::json::wvalue e; e["error"] = "Campo: ip"; return crow::response(400, e.dump());
        }
        std::string ip = (std::string)j["ip"].s();
        BanScore::unban(ip);
        crow::json::wvalue x;
        x["status"] = "unbanned";
        x["ip"]     = ip;
        return crow::response(x.dump());
    });

    // ── Regtest / Network Modes ────────────────────────────────────────────────
    // GET /regtest/info — regtest mode info and parameters
    CROW_ROUTE(app, "/regtest/info")
    ([]{
        Metrics::incApiRequest();
        auto params = Regtest::getParams();
        crow::json::wvalue x;
        x["mode"]             = params.name;
        x["address_prefix"]   = params.addressPrefix;
        x["magic"]            = params.magic;
        x["min_difficulty"]   = params.minDifficulty;
        x["halving_interval"] = params.halvingInterval;
        x["initial_reward"]   = params.initialReward;
        x["instant_mine"]     = params.instantMine;
        x["is_regtest"]       = Regtest::isRegtest();
        x["is_testnet"]       = Regtest::isTestnet();
        x["is_mainnet"]       = Regtest::isMainnet();
        return crow::response(x.dump());
    });

    // GET /regtest/generate/<n>/<address> — mine N blocks instantly (regtest only)
    CROW_ROUTE(app, "/regtest/generate/<int>/<string>")
    ([&bc, &p2p](int n, std::string addr){
        Metrics::incApiRequest();
        if (!Regtest::isRegtest() && !Regtest::isTestnet()) {
            crow::json::wvalue e;
            e["error"] = "Apenas disponível em regtest ou testnet";
            return crow::response(403, e.dump());
        }
        if (n < 1 || n > 1000) {
            crow::json::wvalue e; e["error"] = "n deve estar entre 1 e 1000";
            return crow::response(400, e.dump());
        }
        crow::json::wvalue::list hashes;
        for (int i = 0; i < n; i++) {
            async_mine(bc, p2p, addr);
            hashes.push_back(bc.getLastBlock().hash);
        }
        crow::json::wvalue x;
        x["generated"] = n;
        x["address"]   = addr;
        x["blocks"]    = std::move(hashes);
        x["height"]    = bc.getHeight();
        return crow::response(x.dump());
    });

    // GET /upgrades — network upgrades status
    CROW_ROUTE(app, "/upgrades")
    ([&bc]{
        Metrics::incApiRequest();
        auto upgrades = Regtest::getUpgrades(bc.getHeight());
        crow::json::wvalue::list ul;
        for (const auto& u : upgrades) {
            crow::json::wvalue uv;
            uv["name"]              = u.name;
            uv["activation_height"] = u.activationHeight;
            uv["active"]            = u.active;
            ul.push_back(std::move(uv));
        }
        crow::json::wvalue x;
        x["upgrades"] = std::move(ul);
        x["height"]   = bc.getHeight();
        return crow::response(x.dump());
    });

    // ── Config ─────────────────────────────────────────────────────────────────
    // GET /config — current node configuration (non-sensitive fields)
    CROW_ROUTE(app, "/config")
    ([]{
        Metrics::incApiRequest();
        const auto& cfg = Config::get();
        crow::json::wvalue x;
        x["port"]            = cfg.port;
        x["maxpeers"]        = cfg.maxPeers;
        x["testnet"]         = cfg.testnet;
        x["regtest"]         = cfg.regtest;
        x["mainnet"]         = cfg.mainnet;
        x["mine"]            = cfg.mine;
        x["datadir"]         = cfg.datadir;
        x["prune"]           = cfg.prune;
        x["prune_target_mb"] = cfg.pruneTarget;
        x["txindex"]         = cfg.txindex;
        x["addrindex"]       = cfg.addrindex;
        x["max_mempool_mb"]  = cfg.maxMempoolMB;
        x["min_relay_fee"]   = cfg.minRelayFee;
        x["allow_rbf"]       = cfg.allowRBF;
        x["rpc_enabled"]     = cfg.rpcEnabled;
        x["rpc_port"]        = cfg.rpcPort;
        x["log_level"]       = cfg.logLevel;
        x["segwit"]          = cfg.segwit;
        x["taproot"]         = cfg.taproot;
        x["db_cache_mb"]     = cfg.dbCache;
        x["confirm_target"]  = cfg.confirmTarget;
        return crow::response(x.dump());
    });

    // ── Wallet Features ────────────────────────────────────────────────────────
    // GET /wallet/labels — all address labels
    CROW_ROUTE(app, "/wallet/labels")
    ([]{
        Metrics::incApiRequest();
        auto labels = WalletFeatures::getAllLabels();
        crow::json::wvalue::list jl;
        for (const auto& l : labels) {
            crow::json::wvalue lv;
            lv["address"]    = l.address;
            lv["label"]      = l.label;
            lv["watch_only"] = l.watchOnly;
            lv["created_at"] = l.createdAt;
            jl.push_back(std::move(lv));
        }
        crow::json::wvalue x;
        x["labels"] = std::move(jl);
        x["count"]  = (int)labels.size();
        return crow::response(x.dump());
    });

    // POST /wallet/label — set address label
    CROW_ROUTE(app, "/wallet/label").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        Metrics::incApiRequest();
        auto j = crow::json::load(req.body);
        if (!j || !j.has("address") || !j.has("label")) {
            crow::json::wvalue e; e["error"] = "Campos: address, label"; return crow::response(400, e.dump());
        }
        std::string addr  = (std::string)j["address"].s();
        std::string label = (std::string)j["label"].s();
        bool watchOnly    = j.has("watch_only") && j["watch_only"].b();
        WalletFeatures::setLabel(addr, label, watchOnly);
        WalletFeatures::saveLabels("data/labels.dat");
        crow::json::wvalue x;
        x["status"]     = "saved";
        x["address"]    = addr;
        x["label"]      = label;
        x["watch_only"] = watchOnly;
        return crow::response(x.dump());
    });

    // GET /wallet/history/<address> — transaction history for address
    CROW_ROUTE(app, "/wallet/history/<string>")
    ([&bc](std::string addr){
        Metrics::incApiRequest();
        auto history = WalletFeatures::getTxHistory(addr);
        // Also scan blockchain
        auto chain = bc.getChain();
        for (const auto& blk : chain) {
            for (const auto& tx : blk.transactions) {
                bool relevant = false;
                WalletFeatures::TxRecord rec;
                rec.txid        = tx.id.empty() ? tx.hash : tx.id;
                rec.blockHeight = blk.index;
                rec.timestamp   = blk.timestamp;
                rec.fee         = 0;
                for (const auto& vout : tx.vout) {
                    if (vout.address == addr) {
                        rec.incoming = true;
                        rec.amount   = vout.amount;
                        rec.to       = addr;
                        relevant     = true;
                    }
                }
                if (relevant) history.push_back(rec);
            }
        }
        crow::json::wvalue::list jl;
        for (const auto& t : history) {
            crow::json::wvalue tv;
            tv["txid"]        = t.txid;
            tv["amount"]      = t.amount;
            tv["incoming"]    = t.incoming;
            tv["block_height"]= t.blockHeight;
            tv["timestamp"]   = t.timestamp;
            tv["label"]       = t.label;
            jl.push_back(std::move(tv));
        }
        crow::json::wvalue x;
        x["address"] = addr;
        x["history"] = std::move(jl);
        x["count"]   = (int)jl.size();
        return crow::response(x.dump());
    });

    // GET /wallet/accounts — list multi-account wallets
    CROW_ROUTE(app, "/wallet/accounts")
    ([]{
        Metrics::incApiRequest();
        auto accounts = WalletFeatures::getAccounts();
        crow::json::wvalue::list jl;
        for (const auto& a : accounts) {
            crow::json::wvalue av;
            av["index"]      = a.index;
            av["name"]       = a.name;
            av["root_addr"]  = a.rootAddress;
            av["watch_only"] = a.watchOnly;
            av["created_at"] = a.createdAt;
            jl.push_back(std::move(av));
        }
        crow::json::wvalue x;
        x["accounts"] = std::move(jl);
        x["active"]   = WalletFeatures::getActiveAccount();
        return crow::response(x.dump());
    });

    // POST /psbt/create — create PSBT structure
    CROW_ROUTE(app, "/psbt/create").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        Metrics::incApiRequest();
        auto j = crow::json::load(req.body);
        if (!j || !j.has("inputs") || !j.has("outputs")) {
            crow::json::wvalue e; e["error"] = "Campos: inputs (array), outputs (array)";
            return crow::response(400, e.dump());
        }
        WalletFeatures::PSBT psbt;
        for (int i = 0; i < (int)j["inputs"].size(); i++) {
            WalletFeatures::PSBTInput inp;
            inp.txid   = (std::string)j["inputs"][i]["txid"].s();
            inp.vout   = (int)j["inputs"][i]["vout"].i();
            psbt.inputs.push_back(inp);
        }
        for (int i = 0; i < (int)j["outputs"].size(); i++) {
            WalletFeatures::PSBTOutput out;
            out.address = (std::string)j["outputs"][i]["address"].s();
            out.amount  = j["outputs"][i]["amount"].d();
            psbt.outputs.push_back(out);
        }
        crow::json::wvalue x;
        x["psbt"]      = psbt.serialize();
        x["inputs"]    = (int)psbt.inputs.size();
        x["outputs"]   = (int)psbt.outputs.size();
        x["finalized"] = psbt.finalized;
        x["complete"]  = psbt.isComplete();
        return crow::response(x.dump());
    });

    // ── Crypto Utilities ───────────────────────────────────────────────────────
    // POST /crypto/hash160 — hash160(data) = RIPEMD160(SHA256(data))
    CROW_ROUTE(app, "/crypto/hash160").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        Metrics::incApiRequest();
        auto j = crow::json::load(req.body);
        if (!j || !j.has("hex")) {
            crow::json::wvalue e; e["error"] = "Campo: hex"; return crow::response(400, e.dump());
        }
        try {
            auto data = ECRecovery::fromHex((std::string)j["hex"].s());
            auto h    = ECRecovery::hash160(data);
            crow::json::wvalue x;
            x["hash160"]  = ECRecovery::toHex(h);
            x["ripemd160"]= ECRecovery::toHex(ECRecovery::ripemd160(data));
            x["sha256d"]  = ECRecovery::toHex(ECRecovery::sha256d(data));
            return crow::response(x.dump());
        } catch (const std::exception& e) {
            crow::json::wvalue err; err["error"] = std::string(e.what());
            return crow::response(400, err.dump());
        }
    });

    // POST /crypto/recover — recover pubkey from ECDSA signature
    CROW_ROUTE(app, "/crypto/recover").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        Metrics::incApiRequest();
        auto j = crow::json::load(req.body);
        if (!j || !j.has("sig") || !j.has("hash")) {
            crow::json::wvalue e; e["error"] = "Campos: sig (DER hex), hash (32-byte hex)";
            return crow::response(400, e.dump());
        }
        try {
            auto sig  = ECRecovery::fromHex((std::string)j["sig"].s());
            auto hash = ECRecovery::fromHex((std::string)j["hash"].s());
            int  rid  = j.has("recovery_id") ? (int)j["recovery_id"].i() : 0;
            std::string pubkey = ECRecovery::recoverPubkey(sig, hash, rid);
            crow::json::wvalue x;
            x["pubkey"]      = pubkey;
            x["recovered"]   = !pubkey.empty();
            x["recovery_id"] = rid;
            return crow::response(x.dump());
        } catch (const std::exception& e) {
            crow::json::wvalue err; err["error"] = std::string(e.what());
            return crow::response(400, err.dump());
        }
    });

    // POST /crypto/verify — verify ECDSA signature
    CROW_ROUTE(app, "/crypto/verify").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        Metrics::incApiRequest();
        auto j = crow::json::load(req.body);
        if (!j || !j.has("pubkey") || !j.has("hash") || !j.has("sig")) {
            crow::json::wvalue e; e["error"] = "Campos: pubkey, hash, sig (todos hex)";
            return crow::response(400, e.dump());
        }
        bool ok = ECRecovery::verifySig(
            (std::string)j["pubkey"].s(),
            (std::string)j["hash"].s(),
            (std::string)j["sig"].s()
        );
        crow::json::wvalue x;
        x["valid"] = ok;
        return crow::response(x.dump());
    });

    // ── Bloom Filter (SPV) ─────────────────────────────────────────────────────
    // POST /bloom/create — create a bloom filter
    CROW_ROUTE(app, "/bloom/create").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        Metrics::incApiRequest();
        auto j = crow::json::load(req.body);
        int   n   = j.has("elements")  ? (int)j["elements"].i()    : 100;
        double fp = j.has("fp_rate")   ? j["fp_rate"].d()          : 0.001;
        int   tweak = j.has("tweak")   ? (int)j["tweak"].i()       : 0;
        BloomFilter::Filter f(n, fp, (uint32_t)tweak);
        if (j.has("items")) {
            for (int i = 0; i < (int)j["items"].size(); i++)
                f.insert((std::string)j["items"][i].s());
        }
        crow::json::wvalue x;
        x["hex"]          = f.toHex();
        x["size_bytes"]   = (int)f.sizeBytes();
        x["hash_funcs"]   = (int)f.nHashFuncs;
        x["tweak"]        = (int)f.nTweak;
        x["fp_rate_est"]  = f.falsePositiveRate();
        return crow::response(x.dump());
    });

    // POST /bloom/test — test item membership in bloom filter
    CROW_ROUTE(app, "/bloom/test").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        Metrics::incApiRequest();
        auto j = crow::json::load(req.body);
        if (!j || !j.has("filter") || !j.has("item")) {
            crow::json::wvalue e; e["error"] = "Campos: filter (hex), item (string), hash_funcs, tweak";
            return crow::response(400, e.dump());
        }
        try {
            BloomFilter::Filter f;
            std::string hexFilter = (std::string)j["filter"].s();
            for (size_t i = 0; i+1 < hexFilter.size(); i+=2)
                f.data.push_back((uint8_t)std::stoi(hexFilter.substr(i,2), nullptr, 16));
            f.nHashFuncs = j.has("hash_funcs") ? (uint32_t)j["hash_funcs"].i() : 11;
            f.nTweak     = j.has("tweak")      ? (uint32_t)j["tweak"].i()      : 0;
            std::string item = (std::string)j["item"].s();
            crow::json::wvalue x;
            x["contains"] = f.contains(item);
            x["item"]     = item;
            return crow::response(x.dump());
        } catch (const std::exception& e) {
            crow::json::wvalue err; err["error"] = std::string(e.what());
            return crow::response(400, err.dump());
        }
    });

    // ── Mining / Stratum ───────────────────────────────────────────────────────
    // GET /stratum/stats — pool stats
    CROW_ROUTE(app, "/stratum/stats")
    ([]{
        Metrics::incApiRequest();
        auto res = crow::response(200, Stratum::statsJSON());
        res.set_header("Content-Type", "application/json");
        return res;
    });

    // GET /mining/template/<address> — enhanced block template with ExtraNonce
    CROW_ROUTE(app, "/mining/template/<string>")
    ([&bc](std::string miner_addr){
        Metrics::incApiRequest();
        bc.adjustDifficulty();
        int    height   = bc.getHeight();
        auto   prevHash = bc.getLastBlock().hash;
        int    diff     = bc.getDifficulty();
        double reward   = bc.getBlockReward(height);
        double remaining= 20000000.0 - bc.getTotalSupply();
        if (reward > remaining) reward = (remaining > 0 ? remaining : 0);

        ExtraNonce::nextExtraNonce1();
        std::string en1 = ExtraNonce::extraNonce1Hex();
        std::string en2 = ExtraNonce::extraNonce2Hex();
        std::string coinbaseTxId = ExtraNonce::buildCoinbaseTxId(height, miner_addr, reward,
            ExtraNonce::g_extraNonce1.load(), ExtraNonce::g_extraNonce2.load());

        crow::json::wvalue x;
        x["height"]       = height;
        x["prev_hash"]    = prevHash;
        x["difficulty"]   = diff;
        x["target"]       = std::string(diff, '0');
        x["reward"]       = reward;
        x["miner"]        = miner_addr;
        x["extra_nonce1"] = en1;
        x["extra_nonce2"] = en2;
        x["coinbase_txid"]= coinbaseTxId;
        x["timestamp"]    = (long long)std::time(nullptr);
        x["version"]      = VersionBits::buildVersion((int64_t)std::time(nullptr));
        return crow::response(x.dump());
    });

    // ── RPC Endpoint (JSON-RPC 2.0) ────────────────────────────────────────────
    // POST /rpc — authenticated JSON-RPC
    CROW_ROUTE(app, "/rpc").methods(crow::HTTPMethod::POST)
    ([&bc](const crow::request& req){
        Metrics::incApiRequest();
        // Check auth
        std::string auth = req.get_header_value("Authorization");
        if (!auth.empty() && !RPC::authenticate(auth)) {
            return crow::response(401, "{\"error\":\"Unauthorized\"}");
        }
        // Register methods if not already done
        if (RPC::g_methods.empty()) {
            RPC::registerMethod("getblockcount", [&bc](const RPC::RPCRequest& r) {
                return RPC::RPCResponse{r.id, std::to_string(bc.getHeight()), "", false};
            });
            RPC::registerMethod("getbestblockhash", [&bc](const RPC::RPCRequest& r) {
                return RPC::RPCResponse{r.id, "\"" + bc.getLastBlock().hash + "\"", "", false};
            });
            RPC::registerMethod("getdifficulty", [&bc](const RPC::RPCRequest& r) {
                return RPC::RPCResponse{r.id, std::to_string(bc.getDifficulty()), "", false};
            });
            RPC::registerMethod("getsupply", [&bc](const RPC::RPCRequest& r) {
                return RPC::RPCResponse{r.id, std::to_string(bc.getTotalSupply()), "", false};
            });
            RPC::registerMethod("getbalance", [&bc](const RPC::RPCRequest& r) {
                if (r.params.empty()) return RPC::RPCResponse{r.id, "", "address required", true};
                return RPC::RPCResponse{r.id, std::to_string(bc.getBalance(r.params[0])), "", false};
            });
            RPC::registerMethod("getblocktemplate", [&bc](const RPC::RPCRequest& r) {
                int h = bc.getHeight();
                std::ostringstream ss;
                ss << "{\"height\":" << h
                   << ",\"prevhash\":\"" << bc.getLastBlock().hash << "\""
                   << ",\"difficulty\":" << bc.getDifficulty()
                   << ",\"reward\":" << bc.getBlockReward(h) << "}";
                return RPC::RPCResponse{r.id, ss.str(), "", false};
            });
            RPC::registerMethod("stop", [](const RPC::RPCRequest& r) {
                return RPC::RPCResponse{r.id, "\"MazeChain stopping...\"", "", false};
            });
            RPC::registerMethod("uptime", [](const RPC::RPCRequest& r) {
                return RPC::RPCResponse{r.id, Metrics::toJSON(), "", false};
            });
        }
        std::string resp = RPC::handleRequest(req.body);
        auto res = crow::response(200, resp);
        res.set_header("Content-Type", "application/json");
        return res;
    });

    // ── Locktime / Transaction Info ────────────────────────────────────────────
    // GET /locktime/info — locktime constants and thresholds
    CROW_ROUTE(app, "/locktime/info")
    ([]{
        Metrics::incApiRequest();
        crow::json::wvalue x;
        x["locktime_threshold"]     = (int)Locktime::LOCKTIME_THRESHOLD;
        x["coinbase_maturity"]      = Locktime::COINBASE_MATURITY;
        x["dust_relay_fee_rate"]    = Locktime::DUST_RELAY_FEE_RATE;
        x["p2pkh_input_size"]       = Locktime::P2PKH_INPUT_SIZE;
        x["sequence_final"]         = (long long)Locktime::SEQUENCE_FINAL;
        x["sequence_rbf_max"]       = (long long)Locktime::SEQUENCE_RBF_MAX;
        return crow::response(x.dump());
    });

    // POST /locktime/check — check if a tx is final
    CROW_ROUTE(app, "/locktime/check").methods(crow::HTTPMethod::POST)
    ([&bc](const crow::request& req){
        Metrics::incApiRequest();
        auto j = crow::json::load(req.body);
        if (!j || !j.has("nLockTime")) {
            crow::json::wvalue e; e["error"] = "Campo: nLockTime (uint32)";
            return crow::response(400, e.dump());
        }
        uint32_t nLockTime = (uint32_t)j["nLockTime"].i();
        uint32_t nSeq      = j.has("nSequence") ? (uint32_t)j["nSequence"].i() : 0xffffffff;
        int height         = bc.getHeight();
        int64_t time       = (int64_t)std::time(nullptr);
        bool final_        = Locktime::isFinal(nLockTime, nSeq, height, time);
        bool rbf           = Locktime::signalsRBF(nSeq);
        bool dust          = j.has("amount") && Locktime::isDust(j["amount"].d());
        crow::json::wvalue x;
        x["final"]           = final_;
        x["signals_rbf"]     = rbf;
        x["is_dust"]         = dust;
        x["current_height"]  = height;
        x["current_time"]    = time;
        x["nLockTime"]       = (int)nLockTime;
        x["nSequence"]       = (long long)nSeq;
        return crow::response(x.dump());
    });

    // ── Peer Eviction ──────────────────────────────────────────────────────────
    // GET /peers/detail — detailed peer info
    CROW_ROUTE(app, "/peers/detail")
    ([]{
        Metrics::incApiRequest();
        auto res = crow::response(200, PeerEviction::peersJSON());
        res.set_header("Content-Type", "application/json");
        return res;
    });

    // ── IBD Status ─────────────────────────────────────────────────────────────
    // GET /ibd — initial block download status
    CROW_ROUTE(app, "/ibd")
    ([]{
        Metrics::incApiRequest();
        const auto& ibd = Inventory::ibdState();
        crow::json::wvalue x;
        x["active"]          = ibd.active;
        x["start_height"]    = ibd.startHeight;
        x["target_height"]   = ibd.targetHeight;
        x["current_height"]  = ibd.currentHeight;
        x["progress_pct"]    = ibd.progress * 100.0;
        x["blocks_per_sec"]  = ibd.rate();
        x["best_peer"]       = ibd.bestPeer;
        x["status"]          = ibd.status();
        return crow::response(x.dump());
    });

    // ── Admin Console ──────────────────────────────────────────────────────────
    // GET /admin/status — comprehensive node status for admin
    CROW_ROUTE(app, "/admin/status")
    ([&bc]{
        Metrics::incApiRequest();
        Metrics::setSupply(bc.getTotalSupply());
        Metrics::setDifficulty(bc.getDifficulty());
        crow::json::wvalue x;
        x["version"]         = "4.0.0";
        x["height"]          = bc.getHeight();
        x["best_hash"]       = bc.getLastBlock().hash;
        x["difficulty"]      = bc.getDifficulty();
        x["supply"]          = bc.getTotalSupply();
        x["reward"]          = bc.getBlockReward(bc.getHeight());
        x["metrics"]         = crow::json::load(Metrics::toJSON());
        x["softforks"]       = crow::json::load(VersionBits::toJSON());
        x["network"]         = Regtest::getParams().name;
        x["orphan_txs"]      = OrphanPool::orphanTxCount();
        x["orphan_blocks"]   = OrphanPool::orphanBlockCount();
        x["banned_peers"]    = (int)BanScore::getBannedList().size();
        x["uptime_s"]        = (long long)(std::time(nullptr) - Metrics::g_startTime);
        return crow::response(x.dump());
    });

    int port = 10000;
    const char* port_env = std::getenv("PORT");
    if (port_env && port_env[0] != '\0') {
        try { 
            int p = std::stoi(port_env);
            if (p > 0 && p < 65536) port = p;
        } catch (...) {}
    }
    std::cout << "\n🚀 MAZECHAIN ONLINE | PORTA: " << port << std::endl;

    app.port(port).concurrency(16).run();

    return 0;
}