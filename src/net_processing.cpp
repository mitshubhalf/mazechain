#include <net_processing.h>
#include <blockchain.h>
#include <crypto/sha256.h>
#include <pow.h>
#include <net.h>
#include <consensus/validation.h>
#include <logging.h>
#include <util/time.h>

/**
 * O NetEvents do Bitcoin Core na MazeChain é o "Maestro" que 
 * decide se aceitamos um bloco ou se o minerador deve parar.
 */

namespace {
    // Limite de blocos em voo (in-flight) para evitar ataques de DoS
    static constexpr int MAX_BLOCKS_IN_TRANSIT_PER_PEER = 16;
    static constexpr int64_t BLOCK_DOWNLOAD_TIMEOUT = 60; // segundos
}

class Peer {
public:
    int id;
    std::string address;
    int64_t nLastBlockAnnounce;
    std::vector<uint256> vInventoryToBeSent;

    Peer(int _id, std::string _addr) : id(_id), address(_addr), nLastBlockAnnounce(0) {}
};

/** * Lógica de Recebimento de Mensagens (O Coração do P2P)
 */
bool MazeChainNetHandler::ProcessMessage(CNode& node, const std::string& msg_type, DataStream& vRecv, int64_t time_received, Blockchain& blockchain) {

    LogDebug(BCLog::NET, "Mensagem recebida: %s de peer=%d\n", msg_type, node.GetId());

    // 1. MENSAGEM: VERSION (Handshake inicial)
    if (msg_type == NetMsgType::VERSION) {
        int64_t nTime;
        uint64_t nLocalServices;
        int32_t nVersion;
        vRecv >> nVersion >> nLocalServices >> nTime;

        if (nVersion < MIN_PEER_PROTO_VERSION) {
            LogPrintf("Peer %d usando versão obsoleta. Desconectando.\n", node.GetId());
            node.fDisconnect = true;
            return false;
        }

        // Enviar VERACK (Version Acknowledge)
        blockchain.p2p.PushMessage(node, NetMsgType::VERACK);
        return true;
    }

    // 2. MENSAGEM: INV (Inventory - Quando alguém anuncia um novo bloco ou TX)
    if (msg_type == NetMsgType::INV) {
        std::vector<CInv> vInv;
        vRecv >> vInv;

        for (auto& inv : vInv) {
            if (inv.type == MSG_BLOCK) {
                // Se já temos o bloco, ignoramos a fofoca
                if (blockchain.HasBlock(inv.hash)) continue;

                // Se não temos, pedimos o bloco completo (GETDATA)
                std::vector<CInv> vGetData;
                vGetData.push_back(inv);
                blockchain.p2p.PushMessage(node, NetMsgType::GETDATA, vGetData);

                LogPrintf("Novo bloco anunciado! Hash: %s de peer=%d\n", inv.hash.ToString(), node.GetId());
            }
            else if (inv.type == MSG_TX) {
                if (blockchain.mempool.exists(inv.hash)) continue;
                // Lógica de download de transação omitida para brevidade
            }
        }
        return true;
    }

    // 3. MENSAGEM: BLOCK (Onde a mágica da MazeChain acontece)
    if (msg_type == NetMsgType::BLOCK) {
        Block incomingBlock;
        vRecv >> incomingBlock;

        uint256 hash = incomingBlock.GetHash();

        // VALIDAÇÃO DE CONSENSO (POW.CPP)
        if (!CheckProofOfWork(hash, incomingBlock.nBits, blockchain.GetConsensusParams())) {
            LogPrintf("⚠️ Peer %d enviou bloco com PoW INVÁLIDO! Banindo...\n", node.GetId());
            node.nMisbehavior += 100;
            return false;
        }

        // VERIFICAÇÃO DE SEQUÊNCIA (Anterior/Posterior)
        if (incomingBlock.prevHash != blockchain.GetBestBlockHash()) {
            // Se o bloco não encaixa no topo da chain, pode ser um fork ou estarmos atrasados
            if (blockchain.HasBlock(incomingBlock.prevHash)) {
                 LogPrintf("Recebemos um bloco de uma ramificação paralela (Fork). Index: %d\n", incomingBlock.index);
            } else {
                 // Pedir blocos que faltam (Headers Sync)
                 blockchain.p2p.SyncHeaders(node);
                 return true;
            }
        }

        // ADICIONAR À BLOCKCHAIN E INTERROMPER MINERADOR
        if (blockchain.addBlock(incomingBlock)) {
            LogPrintf("✅ Bloco #%d aceito via Rede! Hash: %s\n", incomingBlock.index, hash.ToString());

            // AÇÃO CRÍTICA: Interromper o minerador local para mudar para o próximo bloco
            blockchain.interruptMining.store(true);

            // Re-transmitir (Relay) para outros peers (Gossip Protocol)
            blockchain.p2p.BroadcastBlock(incomingBlock);

            // Limpar mempool de transações que agora estão no bloco
            blockchain.mempool.RemoveConfirmed(incomingBlock.transactions);
        }

        return true;
    }

    // 4. MENSAGEM: GETBLOCKS (Sincronização de novos nós)
    if (msg_type == NetMsgType::GETBLOCKS) {
        CBlockLocator locator;
        uint256 hashStop;
        vRecv >> locator >> hashStop;

        // O Bitcoin Core usa o Locator para saber de onde começar a enviar blocos
        uint256 startHash = blockchain.FindFork(locator);
        std::vector<uint256> vHashes = blockchain.GetHashesSince(startHash, hashStop);

        // Enviar INV com os hashes para o peer baixar
        std::vector<CInv> vInv;
        for (const auto& h : vHashes) {
            vInv.emplace_back(MSG_BLOCK, h);
            if (vInv.size() >= 500) break; // Lote máximo
        }
        blockchain.p2p.PushMessage(node, NetMsgType::INV, vInv);
        return true;
    }

    return true;
}

/**
 * Gerenciador de Estados dos Peers
 * Responsável por detectar se um minerador vizinho "travou"
 */
void MazeChainNetHandler::CheckForStalls(CNode& node, int64_t current_time) {
    auto state = State(node.GetId());
    if (state->vBlocksInFlight.empty()) return;

    for (auto it = state->vBlocksInFlight.begin(); it != state->vBlocksInFlight.end();) {
        if (current_time - it->nTimeSent > BLOCK_DOWNLOAD_TIMEOUT) {
            LogPrintf("Peer %d demorou muito para entregar o bloco %s. Penalizando.\n", 
                      node.GetId(), it->hash.ToString());
            node.nMisbehavior += 10;
            it = state->vBlocksInFlight.erase(it);
        } else {
            ++it;
        }
    }
}

/**
 * Função de Envio de Mensagens (SendMessages)
 * Chamada periodicamente para manter a rede fluida
 */
bool MazeChainNetHandler::SendMessages(CNode& node, Blockchain& blockchain) {
    int64_t current_time = GetTime();

    // 1. Manter conexão viva (PING)
    if (current_time - node.nLastSend > 30) {
        blockchain.p2p.PushMessage(node, NetMsgType::PING, current_time);
    }

    // 2. Anunciar novos itens (TXs e Blocos) que mineramos ou recebemos
    if (!node.vInventoryToBeSent.empty()) {
        std::vector<CInv> vInv;
        {
            LOCK(node.cs_inventory);
            vInv.swap(node.vInventoryToBeSent);
        }
        blockchain.p2p.PushMessage(node, NetMsgType::INV, vInv);
    }

    // 3. Sincronização inicial (se estivermos atrás da rede)
    if (blockchain.IsInitialBlockDownload() && current_time - node.nLastSyncRequest > 10) {
        blockchain.p2p.RequestHeaders(node, blockchain.GetLocator());
        node.nLastSyncRequest = current_time;
    }

    return true;
}

// Implementação do Handshake Verack
bool MazeChainNetHandler::ProcessVerack(CNode& node, Blockchain& blockchain) {
    LogPrintf("Verack recebido de %d. Peer totalmente conectado.\n", node.GetId());
    node.fSuccessfullyConnected = true;

    // Assim que conectamos, perguntamos pelos blocos dele
    blockchain.p2p.PushMessage(node, NetMsgType::GETBLOCKS, blockchain.GetLocator(), uint256());
    return true;
}

/**
 * Verificação de Integridade de Transação na Rede
 * Antes de colocar na mempool, o net_processing verifica a taxa
 */
bool MazeChainNetHandler::AcceptToMemoryPool(const Transaction& tx, Blockchain& blockchain) {
    if (!blockchain.verifyTransaction(tx)) return false;

    // Verificar se a taxa é compatível com o relay mínimo da MazeChain
    double fee = blockchain.calculateFee(tx);
    if (fee < blockchain.GetMinRelayFee()) {
        return false;
    }

    return blockchain.mempool.add(tx);
}