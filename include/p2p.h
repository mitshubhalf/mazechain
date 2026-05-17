#ifndef P2P_H
#define P2P_H

#include <vector>
#include <string>
#include <set>

class Blockchain; // forward declaration

class P2P {
public:
    std::set<std::string> peers;

    void add_peer(std::string peer_url);
    void remove_peer(const std::string& peer_url);
    void save_peers(const std::string& filename);
    void load_peers(const std::string& filename);

    void broadcast_block(const std::string& block_json);
    void broadcast_transaction(const std::string& tx_json);

    // Download blocks from peers and apply to local chain
    void sync_with_peers(Blockchain& bc, const std::string& db_path);
};

#endif
