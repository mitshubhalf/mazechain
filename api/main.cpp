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

const std::string ABS_DATA_PATH = "data";
const std::string ABS_WALLET_PATH = "data/wallet.dat";
const std::string MEMPOOL_PATH = "data/mempool.dat";
const std::string DB_PATH = "data/blockchain.dat"; // Caminho consistente para o banco de dados

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
        std::cout << "\n[MOTOR] ⛏️ Minerando Bloco #" << altura_antes 
                  << " para o endereço: " << addr << std::endl;

        bc.mineBlock(addr);

        int altura_depois = (int)bc.getChain().size();
        if (altura_depois > altura_antes) {
            std::cout << "[MOTOR] ✅ Bloco " << (altura_depois - 1) 
                      << " Minerado com Sucesso!" << std::endl;

            // Broadcast full block JSON to peers
            const auto& chain = bc.getChain();
            if (!chain.empty()) {
                std::string block_json = block_to_json_str(chain.back());
                p2p.broadcast_block(block_json);
            }
        } else {
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
            std::cout << "  mine [addr]                    : Minera um bloco manualmente" << std::endl;
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

    std::string local_miner_address = miner_wallet.getAddress(); 
    std::cout << "💳 Endereço do Nó Ativo: " << local_miner_address << std::endl;

    NodeManager node_manager(bc, p2p);

    // Load persisted peers and start background sync
    p2p.load_peers("data/peers.dat");
    if (!p2p.peers.empty()) {
        std::thread([&bc, &p2p](){
            std::this_thread::sleep_for(std::chrono::seconds(3)); // wait for server to start
            p2p.sync_with_peers(bc, DB_PATH);
        }).detach();
    }

    std::thread auto_miner(
        worker_mineracao_continua, 
        std::ref(bc), 
        std::ref(p2p), 
        local_miner_address
    );
    auto_miner.detach();

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
        crow::json::wvalue x;
        x["protocol"]["difficulty"] = bc.getDifficulty();
        x["protocol"]["total_supply"] = bc.getTotalSupply();
        x["protocol"]["max_supply"] = bc.getMaxSupply();
        x["chain"]["height"] = bc.getHeight();
        x["network"]["peers_count"] = (int)p2p.peers.size();
        x["node"]["version"] = "2.1.0";
        return x;
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

    int port = 10000;
    const char* port_env = std::getenv("PORT");
    if (port_env && port_env[0] != '\0') {
        try { 
            int p = std::stoi(port_env);
            if (p > 0 && p < 65536) port = p;
        } catch (...) {}
    }
    std::cout << "\n🚀 MAZECHAIN ONLINE | PORTA: " << port << std::endl;

    app.port(port).multithreaded().run();

    return 0;
}