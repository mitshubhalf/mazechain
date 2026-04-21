#include "../include/node_manager.h"
#include <iostream>

// Definição do Construtor
NodeManager::NodeManager(Blockchain& bc, P2P& p) : chain(bc), p2p(p) {}

bool NodeManager::handle_incoming_block(const Block& newBlock) {
    // 1. Verifica sequência
    if (newBlock.index != chain.getLastBlock().index + 1) {
        std::cout << "⚠️ Bloco fora de sequência. Ignorado." << std::endl;
        return false;
    }

    // 2. Verifica hash anterior
    if (newBlock.prevHash != chain.getLastBlock().hash) {
        std::cout << "⚠️ Erro de integridade: Hash anterior não confere." << std::endl;
        return false;
    }

    // 3. Adiciona e propaga
    chain.addBlock(newBlock);
    p2p.broadcast_block(newBlock.hash);

    return true;
}

void NodeManager::sync_network() {
    std::cout << "🔄 Sincronizando com a rede MazeChain..." << std::endl;
    // p2p.sync_with_peers(); // Ative se a função existir no p2p.h
}