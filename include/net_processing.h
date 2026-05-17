// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Copyright (c) 2026-present The MazeChain developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MAZECHAIN_NET_PROCESSING_H
#define MAZECHAIN_NET_PROCESSING_H

#include <net.h>
#include <protocol.h>
#include <uint256.h>
#include <consensus/params.h>
#include <sync.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class Blockchain;
class CTxMemPool;
class BanMan;

/** Altura padrão para considerar um nó sincronizado na MazeChain */
static const unsigned int MAX_HEADERS_RESULTS = 2000;
/** Tempo limite para um peer entregar um bloco solicitado antes de ser considerado "stalling" */
static constexpr std::chrono::seconds BLOCK_DOWNLOAD_TIMEOUT{60};

/**
 * Estatísticas de estado de um nó (Peer) na MazeChain.
 * Usado pela API e pelo NodeManager para monitorar a saúde da rede.
 */
struct CNodeStateStats {
    int nSyncHeight = -1;           // Altura atual do peer
    int nCommonHeight = -1;         // Último bloco comum entre nós e o peer
    std::vector<int> vHeightInFlight; // Blocos que pedimos e estamos esperando
    bool m_relay_txs;               // Se o peer aceita retransmitir transações
    int m_inv_to_send = 0;          // Quantidade de inventário na fila para este peer
    uint64_t m_addr_processed = 0;
    ServiceFlags their_services;    // Serviços oferecidos pelo nó (NODE_NETWORK, etc)
};

/**
 * PeerManager: O Coração do Protocolo P2P.
 * Esta classe abstrata define como as mensagens de rede interagem com a lógica da MazeChain.
 */
class PeerManager : public NetEventsInterface
{
public:
    struct Options {
        bool ignore_incoming_txs{false}; // Modo -blocksonly
        uint32_t max_extra_txs{100};     // Limite de órfãos na mempool
        bool capture_messages{false};    // Dump de tráfego para debug
    };

    /** Fábrica para criar a instância do PeerManager */
    static std::unique_ptr<PeerManager> make(CConnman& connman, BanMan* banman, 
                                            Blockchain& blockchain, CTxMemPool& pool, 
                                            Options opts);

    virtual ~PeerManager() = default;

    /** * Sincronização: Tenta buscar um bloco específico de um peer.
     * Útil para recuperar blocos perdidos ou forks.
     */
    virtual bool FetchBlock(NodeId peer_id, const uint256& block_hash) = 0;

    /** * GetNodeStateStats: Retorna dados técnicos sobre a conexão com um Peer específico.
     */
    virtual bool GetNodeStateStats(NodeId nodeid, CNodeStateStats& stats) const = 0;

    /** * InitiateBlockBroadcast: Avisa a todos os peers sobre um bloco que acabamos de minerar.
     */
    virtual void InitiateBlockBroadcast(const uint256& block_hash) = 0;

    /** * InitiateTxBroadcast: Espalha uma transação da nossa mempool para a rede.
     */
    virtual void InitiateTxBroadcast(const uint256& txid) = 0;

    /** * SetBestBlock: Atualiza o PeerManager sobre o topo da nossa blockchain local.
     * Essencial para o PeerManager saber quais blocos novos deve pedir.
     */
    virtual void SetBestBlock(int height, const uint256& block_hash) = 0;

    /** * Misbehaving: Penaliza um nó que enviou dados inválidos (ex: PoW errado).
     * Se o score de banimento ultrapassar o limite, o peer é desconectado.
     */
    virtual void Misbehaving(NodeId peer_id, int how_much, const std::string& message) = 0;

    /**
     * CheckForStaleTip: Verifica se a nossa ponta da chain está "velha" (sem blocos novos há muito tempo).
     * Se estiver, tenta forçar conexões com novos peers para sincronizar.
     */
    virtual void CheckForStaleTipAndEvictPeers() = 0;

    /**
     * ProcessMessages: A função principal chamada pelo loop de rede (net.cpp).
     * É aqui que o bloco recebido entra, é validado pelo POW.CPP e onde chamamos:
     * blockchain.interruptMining = true;
     */
    virtual bool ProcessMessages(CNode* pfrom, std::atomic<bool>& interruptMsgProc) override = 0;

    /**
     * SendMessages: Envia pings, inventários (INV) e solicitações (GETDATA) pendentes.
     */
    virtual bool SendMessages(CNode* pto) override = 0;
};

#endif // MAZECHAIN_NET_PROCESSING_H