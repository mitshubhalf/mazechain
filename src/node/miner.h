// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Copyright (c) 2026-present The MazeChain developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MAZECHAIN_NODE_MINER_H
#define MAZECHAIN_NODE_MINER_H

#include <interfaces/types.h>
#include <node/types.h>
#include <policy/policy.h>
#include <primitives/block.h>
#include <txmempool.h>
#include <util/feefrac.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

class ArgsManager;
class CBlockIndex;
class CChainParams;
class CScript;
class Chainstate;
class ChainstateManager;

namespace Consensus { struct Params; };

using interfaces::BlockRef;

namespace node {
class KernelNotifications;

/** * MAZECHAIN - CBlockTemplate
 * Estrutura que contém o bloco em construção e os metadados de taxas.
 */
struct CBlockTemplate
{
    CBlock block;
    // Taxas por transação (excluindo a coinbase). 
    // Na MazeChain, estas taxas serão destinadas ao Fundo de Reserva.
    std::vector<CAmount> vTxFees;
    std::vector<int64_t> vTxSigOpsCost;
    std::vector<FeePerVSize> m_package_feerates;

    // Campos da coinbase configurados pelo minerador (inclui a divisão de recompensas)
    CoinbaseTx m_coinbase_tx;
};

/** * MAZECHAIN - BlockAssembler
 * O "Arquiteto" do bloco. Ele seleciona as transações e aplica as leis da MazeChain.
 */
class BlockAssembler
{
private:
    std::unique_ptr<CBlockTemplate> pblocktemplate;

    // Status do bloco atual
    uint64_t nBlockWeight;
    uint64_t nBlockTx;
    uint64_t nBlockSigOpsCost;
    CAmount nFees; // Total de taxas que irá para o Fundo de Reserva

    int nHeight;
    int64_t m_lock_time_cutoff;

    const CChainParams& chainparams;
    const CTxMemPool* const m_mempool;
    Chainstate& m_chainstate;

public:
    struct Options : BlockCreateOptions {
        size_t nBlockMaxWeight{DEFAULT_BLOCK_MAX_WEIGHT};
        CFeeRate blockMinFeeRate{DEFAULT_BLOCK_MIN_TX_FEE};
        bool test_block_validity{true};
        bool print_modified_fee{false};
    };

    explicit BlockAssembler(Chainstate& chainstate, const CTxMemPool* mempool, const Options& options);

    /** Constrói o novo template do bloco seguindo a regra: 400 MZ (Miner) + Taxas (Reserva) */
    std::unique_ptr<CBlockTemplate> CreateNewBlock();

    // Estatísticas do último bloco
    inline static std::optional<int64_t> m_last_block_num_txs{};
    inline static std::optional<int64_t> m_last_block_weight{};

private:
    const Options m_options;

    void resetBlock();
    void AddToBlock(const CTxMemPoolEntry& entry);
    void addChunks() EXCLUSIVE_LOCKS_REQUIRED(m_mempool->cs);

    bool TestChunkBlockLimits(FeePerWeight chunk_feerate, int64_t chunk_sigops_cost) const;
    bool TestChunkTransactions(const std::vector<CTxMemPoolEntryRef>& txs) const;
};

/**
 * MAZECHAIN - GetMinimumTime
 * Retorna o tempo mínimo para o próximo bloco, garantindo a estabilidade da rede.
 */
int64_t GetMinimumTime(const CBlockIndex* pindexPrev, int64_t difficulty_adjustment_interval);

/**
 * Atualiza o timestamp e recalcula a dificuldade necessária (Níveis 4, 6 ou Dinâmico).
 */
int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev);

/** Funções de utilidade para Merkle Root e compromissos de Testemunha (SegWit) */
void RegenerateCommitments(CBlock& block, ChainstateManager& chainman);
void AddMerkleRootAndCoinbase(CBlock& block, CTransactionRef coinbase, uint32_t version, uint32_t timestamp, uint32_t nonce);

// Funções de sincronização e espera de novos blocos/tips
std::optional<BlockRef> WaitTipChanged(ChainstateManager& chainman, KernelNotifications& kernel_notifications, const uint256& current_tip, MillisecondsDouble& timeout, bool& interrupt);
bool CooldownIfHeadersAhead(ChainstateManager& chainman, KernelNotifications& kernel_notifications, const BlockRef& last_tip, bool& interrupt_mining);

} // namespace node

#endif // MAZECHAIN_NODE_MINER_H