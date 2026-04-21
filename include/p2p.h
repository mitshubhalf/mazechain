#ifndef P2P_H
#define P2P_H

#include <vector>
#include <string>
#include <set>
#include "crow_all.h"

class P2P {
public:
    // Lista de IPs/URLs de outros mineradores
    std::set<std::string> peers;

    // Adiciona um novo nó na lista de conhecidos
    void add_peer(std::string peer_url);

    // Envia um bloco novo para todos os conhecidos
    void broadcast_block(const std::string& block_json);

    // Envia uma transação nova para a rede
    void broadcast_transaction(const std::string& tx_json);

    // Pergunta aos outros nós qual a altura da rede deles
    void sync_with_peers();
};

#endif
