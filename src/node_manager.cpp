#include "../include/node_manager.h"
#include "../include/validation.h" // Necessário para GetMazeRequiredDifficulty e CAmount
#include <iostream>

// Construtor: Inicializa a referência da Blockchain e do P2P
NodeManager::NodeManager(Blockchain& bc, P2P& p) : chain(bc), p2p(p) {}

bool NodeManager::handle_incoming_block(const Block& newBlock) {
    std::cout << "[NÓ] Analisando bloco recebido #" << newBlock.index << "..." << std::endl;

    // 1. Verificação de Sequência (Index)
    if (newBlock.index <= chain.getLastBlock().index) {
        std::cout << "ℹ️ Bloco #" << newBlock.index << " antigo ou duplicado. Ignorado." << std::endl;
        return false;
    }

    if (newBlock.index != chain.getLastBlock().index + 1) {
        std::cout << "⚠️ Bloco fora de sequência. Esperado: " << (chain.getLastBlock().index + 1) 
                  << " Recebido: " << newBlock.index << std::endl;
        return false;
    }

    // 2. Verificação de Integridade (Hash Anterior)
    if (newBlock.prevHash != chain.getLastBlock().hash) {
        std::cout << "⚠️ Erro de integridade: Hash anterior não confere." << std::endl;
        return false;
    }

    // 3. Validação de Consenso (Proof of Work)
    // Simplificamos a chamada removendo o 'chain.getParams()' que não existe no seu motor
    // A dificuldade agora é obtida diretamente pela altura
    unsigned int nTargetDifficulty = GetMazeRequiredDifficulty(newBlock.index, {}); 

    // Verificação visual do alvo de dificuldade
    std::string targetPrefix(nTargetDifficulty, '0');
    if (newBlock.hash.substr(0, nTargetDifficulty) != targetPrefix) {
        std::cout << "❌ ERRO DE CONSENSO: Bloco #" << newBlock.index 
                  << " não atingiu a dificuldade " << nTargetDifficulty << std::endl;
        return false;
    }

    // 4. Persistência
    // Como seu addBlock retorna void, chamamos ele diretamente
    chain.addBlock(newBlock);

    // 5. Feedback e Propagação
    std::cout << "✅ Bloco #" << newBlock.index << " aceito e gravado (Dificuldade: " << nTargetDifficulty << ")" << std::endl;

    // Só propaga se a função existir no seu P2P
    // p2p.broadcast_block(newBlock.hash); 

    return true;
}

void NodeManager::sync_network() {
    std::cout << "🔄 Sincronizando nó MazeChain na altura: " << chain.getLastBlock().index << std::endl;

    // As funções request_block_hashes e download_block foram removidas 
    // porque não estão implementadas ou dão erro de linkagem.
    // O nó irá sincronizar via escuta de novos blocos (P2P dinâmico).

    /* Nota: Para reativar a sincronização histórica, as funções devem ser 
    declaradas no include/p2p.h e implementadas no src/p2p.cpp.
    */
}