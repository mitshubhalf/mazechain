#ifndef P2P_H
#define P2P_H

#include <vector>
#include <string>
#include <set>

class Blockchain; // forward declaration

// ── Nós semente hardcoded (bootstrap discovery) ───────────────────────────────
// Equivalente aos DNS seeds do Bitcoin. Novos nós tentam estes primeiro.
// Adicione o endereço do seu nó de produção aqui quando lançar a mainnet.
static const std::vector<std::string> MAZECHAIN_SEED_NODES = {
    // "https://node1.mazechain.io",
    // "https://node2.mazechain.io",
    // "https://seed.mazechain.io"
    // (adicione nós públicos reais aqui ao lançar mainnet)
};

class P2P {
public:
    std::set<std::string> peers;

    void add_peer(std::string peer_url);
    void remove_peer(const std::string& peer_url);
    void save_peers(const std::string& filename);
    void load_peers(const std::string& filename);

    void broadcast_block(const std::string& block_json);
    void broadcast_transaction(const std::string& tx_json);

    // Sincroniza com todos os peers conhecidos
    void sync_with_peers(Blockchain& bc, const std::string& db_path);

    // ── NOVOS: Handshake de Versão + Descoberta de Peers ─────────────────────

    // Verifica se um peer é compatível (mesma rede, mesmo genesis)
    // Retorna true se handshake OK, false se incompatível (diferente rede/genesis)
    bool version_handshake(const std::string& peer_url, const std::string& local_genesis_hash);

    // Troca lista de peers com um nó: envia os nossos, recebe os dele
    void exchange_peers(const std::string& peer_url);

    // Tenta conectar nos seed nodes hardcoded + arquivo salvo
    void bootstrap(const std::string& peers_file, const std::string& local_genesis_hash);
};

#endif
