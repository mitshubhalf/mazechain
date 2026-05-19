#ifndef CONFIG_FILE_H
#define CONFIG_FILE_H

// MazeChain — Advanced Config File (.mazechain.conf)
// Key=value format, supports comments (#), sections ([section])
// Precedence: CLI flags > env vars > config file > defaults

#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>

namespace Config {

    struct NodeConfig {
        // Network
        int         port             = 10000;
        std::string bind             = "0.0.0.0";
        int         maxPeers         = 32;
        int         maxConnections   = 125;
        bool        upnp             = false;
        bool        tor              = false;
        std::string torProxy         = "127.0.0.1:9050";
        std::string addNode;             // peer to add
        std::string connect;             // only connect to this peer
        bool        listenP2P        = true;
        int         p2pPort          = 10001;

        // Chain
        bool        testnet          = false;
        bool        regtest          = false;
        bool        mainnet          = true;

        // Mining
        bool        mine             = false;
        std::string mineAddr;
        int         mineThreads      = 1;

        // Storage
        std::string datadir          = "data";
        bool        prune            = false;
        int         pruneTarget      = 550;      // MB
        bool        txindex          = true;
        bool        addrindex        = true;
        bool        blockindex       = true;

        // Mempool
        int         maxMempoolMB     = 300;
        double      minRelayFee      = 0.000003;
        bool        allowRBF         = true;
        bool        mempoolReplacement = true;

        // RPC
        bool        rpcEnabled       = true;
        std::string rpcUser          = "mazechain";
        std::string rpcPassword      = "changeme";
        int         rpcPort          = 10002;
        std::string rpcBind          = "127.0.0.1";
        bool        rpcAllowIp       = false;
        std::string rpcAllowIpFrom   = "127.0.0.1";

        // Logging
        std::string logLevel         = "INFO";
        bool        logToFile        = true;
        std::string logFile          = "data/mazechain.log";
        int         logMaxSizeMB     = 10;

        // Wallet
        bool        disableWallet    = false;
        std::string walletFile       = "data/wallet.dat";
        bool        walletEncrypt    = false;
        int         keyPoolSize      = 1000;

        // SegWit / Taproot
        bool        segwit           = true;
        bool        taproot          = true;

        // Performance
        int         dbCache          = 450;      // MB
        int         maxOrphanTx      = 100;
        int         maxOrphanBlock   = 50;
        bool        parallelVerify   = true;

        // Fee
        double      targetFeeRate    = 0.0001;
        int         confirmTarget    = 6;        // blocks for fee estimation
    };

    static NodeConfig g_config;

    inline std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        size_t end   = s.find_last_not_of(" \t\r\n");
        return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
    }

    inline bool parseBool(const std::string& v) {
        std::string lv = v;
        std::transform(lv.begin(), lv.end(), lv.begin(), ::tolower);
        return lv == "1" || lv == "true" || lv == "yes" || lv == "on";
    }

    inline void applyOption(NodeConfig& cfg, const std::string& key, const std::string& val) {
        if (key == "port")             cfg.port = std::stoi(val);
        else if (key == "bind")        cfg.bind = val;
        else if (key == "maxpeers")    cfg.maxPeers = std::stoi(val);
        else if (key == "testnet")     cfg.testnet = parseBool(val);
        else if (key == "regtest")     cfg.regtest = parseBool(val);
        else if (key == "mine")        cfg.mine = parseBool(val);
        else if (key == "mineaddr")    cfg.mineAddr = val;
        else if (key == "minethreads") cfg.mineThreads = std::stoi(val);
        else if (key == "datadir")     cfg.datadir = val;
        else if (key == "prune")       cfg.prune = parseBool(val);
        else if (key == "prunetarget") cfg.pruneTarget = std::stoi(val);
        else if (key == "txindex")     cfg.txindex = parseBool(val);
        else if (key == "addrindex")   cfg.addrindex = parseBool(val);
        else if (key == "maxmempool")  cfg.maxMempoolMB = std::stoi(val);
        else if (key == "minrelayfee") cfg.minRelayFee = std::stod(val);
        else if (key == "rpcuser")     cfg.rpcUser = val;
        else if (key == "rpcpassword") cfg.rpcPassword = val;
        else if (key == "rpcport")     cfg.rpcPort = std::stoi(val);
        else if (key == "rpclisten")   cfg.rpcBind = val;
        else if (key == "loglevel")    cfg.logLevel = val;
        else if (key == "logfile")     cfg.logFile = val;
        else if (key == "walletfile")  cfg.walletFile = val;
        else if (key == "dbcache")     cfg.dbCache = std::stoi(val);
        else if (key == "upnp")        cfg.upnp = parseBool(val);
        else if (key == "tor")         cfg.tor = parseBool(val);
        else if (key == "segwit")      cfg.segwit = parseBool(val);
        else if (key == "taproot")     cfg.taproot = parseBool(val);
        else if (key == "addnode")     cfg.addNode = val;
        else if (key == "connect")     cfg.connect = val;
    }

    inline void load(const std::string& path = "mazechain.conf") {
        std::ifstream f(path);
        if (!f.is_open()) {
            // Try ~/.mazechain/mazechain.conf
            const char* home = std::getenv("HOME");
            if (home) {
                std::string alt = std::string(home) + "/.mazechain/mazechain.conf";
                f.open(alt);
            }
            if (!f.is_open()) return;
        }

        std::string section;
        std::string line;
        while (std::getline(f, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#' || line[0] == ';') continue;
            if (line[0] == '[') {
                section = line.substr(1, line.find(']') - 1);
                continue;
            }
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = trim(line.substr(0, eq));
            std::string val = trim(line.substr(eq+1));
            // Strip inline comment
            size_t comment = val.find('#');
            if (comment != std::string::npos) val = trim(val.substr(0, comment));
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            applyOption(g_config, key, val);
        }
        std::cout << "[CONFIG] Configuração carregada de: " << path << std::endl;
    }

    inline void loadFromEnv() {
        auto getenv_ = [](const char* k) -> std::string {
            const char* v = std::getenv(k);
            return v ? v : "";
        };
        if (auto v = getenv_("MAZE_PORT");     !v.empty()) g_config.port = std::stoi(v);
        if (auto v = getenv_("MAZE_TESTNET");  !v.empty()) g_config.testnet = parseBool(v);
        if (auto v = getenv_("MAZE_REGTEST");  !v.empty()) g_config.regtest = parseBool(v);
        if (auto v = getenv_("MAZE_LOGLEVEL"); !v.empty()) g_config.logLevel = v;
        if (auto v = getenv_("MAZE_DATADIR");  !v.empty()) g_config.datadir = v;
        if (auto v = getenv_("MAZE_MINE");     !v.empty()) g_config.mine = parseBool(v);
        if (auto v = getenv_("MAZE_MINEADDR"); !v.empty()) g_config.mineAddr = v;
    }

    inline NodeConfig& get() { return g_config; }

    // Write default config file
    inline void writeDefault(const std::string& path = "mazechain.conf") {
        std::ofstream f(path);
        f << "# MazeChain Node Configuration\n"
          << "# Generated automatically — edit as needed\n\n"
          << "[network]\n"
          << "port=10000\n"
          << "bind=0.0.0.0\n"
          << "maxpeers=32\n"
          << "upnp=false\n"
          << "p2pport=10001\n\n"
          << "[chain]\n"
          << "testnet=false\n"
          << "regtest=false\n"
          << "segwit=true\n"
          << "taproot=true\n\n"
          << "[mining]\n"
          << "mine=false\n"
          << "# mineaddr=MZyouraddress\n"
          << "minethreads=1\n\n"
          << "[storage]\n"
          << "datadir=data\n"
          << "txindex=true\n"
          << "addrindex=true\n"
          << "dbcache=450\n"
          << "prune=false\n\n"
          << "[mempool]\n"
          << "maxmempool=300\n"
          << "minrelayfee=0.000003\n\n"
          << "[rpc]\n"
          << "rpcuser=mazechain\n"
          << "rpcpassword=changeme\n"
          << "rpcport=10002\n\n"
          << "[logging]\n"
          << "loglevel=INFO\n"
          << "logfile=data/mazechain.log\n\n"
          << "[wallet]\n"
          << "walletfile=data/wallet.dat\n"
          << "keypoolsize=1000\n";
        std::cout << "[CONFIG] Arquivo de configuração padrão criado: " << path << std::endl;
    }

} // namespace Config

#endif
