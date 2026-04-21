#ifndef NODE_MANAGER_H
#define NODE_MANAGER_H

#include "blockchain.h"
#include "p2p.h"
#include "block.h"

class NodeManager {
private:
    Blockchain& chain;
    P2P& p2p;

public:
    // Apenas a promessa do construtor
    NodeManager(Blockchain& bc, P2P& p);

    // Apenas a promessa da função
    void sync_network();

    // Apenas a promessa da função
    bool handle_incoming_block(const Block& newBlock);
};

#endif