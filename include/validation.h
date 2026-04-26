// Copyright (c) 2026 mits
// Copyright (c) 2026-present The Bitcoin Core developers
// Copyright (c) 2026-present The MazeChain developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MAZECHAIN_VALIDATION_H
#define MAZECHAIN_VALIDATION_H

#include <arith_uint256.h>
#include <attributes.h>
#include <chain.h>
#include <checkqueue.h>
#include <coins.h>
#include <consensus/amount.h>
#include <cuckoocache.h>
#include <deploymentstatus.h>
#include <kernel/chain.h>
#include <kernel/chainparams.h>
#include <kernel/chainstatemanager_opts.h>
#include <kernel/cs_main.h>
#include <node/blockstorage.h>
#include <policy/feerate.h>
#include <policy/packages.h>
#include <policy/policy.h>
#include <script/script_error.h>
#include <script/sigcache.h>
#include <script/verify_flags.h>
#include <sync.h>
#include <txdb.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/byte_units.h>
#include <util/check.h>
#include <util/fs.h>
#include <util/hasher.h>
#include <util/result.h>
#include <util/translation.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

class CBlockIndex;
class CBlockTreeDB;
class CChainState;
class CConnman;
class CTxMemPool;
class ChainstateManager;
struct ChainstateRole;

/** Parâmetros de Consenso da MazeChain */
static constexpr CAmount MAZE_INITIAL_REWARD = 400 * COIN;
static constexpr int MAZE_HALVING_INTERVAL = 10000;
static constexpr CAmount MAZE_MAX_SUPPLY = 20000000 * COIN;
extern const std::string MAZE_RESERVE_FUND;

/** Global variable that points to the active block tree (protected by cs_main) */
extern std::unique_ptr<CBlockTreeDB> pblocktree;

/** Global variable that points to the active chainstate manager */
extern std::unique_ptr<ChainstateManager> g_chainman;

/** * MAZECHAIN SUBSIDY LOGIC
 * Implementa as 4 fases de recompensa e a deflação dinâmica.
 */
CAmount GetMazeBlockSubsidy(int nHeight, const Consensus::Params& consensusParams, const CAmount& nTotalSupply, const CAmount& nReserveBalance);

/** * DYNAMIC FEE LOGIC 
 * Calcula a taxa mínima (1% a 7%) baseada na era e no supply.
 */
double GetMazeDynamicFeePercentage(int nHeight, const CAmount& nTotalSupply);

/** * Classe de Validação de Bloco - Padrão Bitcoin Core 28.x
 * Gerencia a aceitação de blocos e verificação de assinaturas.
 */
bool CheckBlock(const CBlock& block, BlockValidationState& state, const Consensus::Params& consensusParams, bool fCheckPOW = true, bool fCheckMerkleRoot = true);

/** Verificação de Proof of Work com piso de Dificuldade 5 após bloco 5000 */
bool CheckProofOfWork(uint256 hash, unsigned int nBits, int nHeight, const Consensus::Params& params);

/**
 * CChainState: O objeto que mantém a base de dados UTXO e a ponta da corrente.
 * Aqui residem as 6.000 linhas de lógica de conexão de blocos.
 */
class Chainstate {
public:
    LOCKABLE_OBJECT(::cs_main, m_chainstate_mutex);

    CChain m_chain;
    std::unique_ptr<CCoinsViewDB> m_block_tree_db;
    std::unique_ptr<CCoinsViewCache> m_coins_views;

    /** * Conecta um bloco validando a "Lei da MazeChain":
     * - Recompensa exata para a altura
     * - Taxas enviadas para o Fundo de Reserva
     * - Maturação de 50 blocos para Coinbase
     */
    bool ConnectBlock(const CBlock& block, BlockValidationState& state, CBlockIndex* pindex,
                      CCoinsViewCache& view, bool fJustCheck = false) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    bool DisconnectBlock(const CBlock& block, const CBlockIndex* pindex, CCoinsViewCache& view);

    /** Atualiza o Supply Total da rede durante a conexão */
    CAmount nChainSupply GUARDED_BY(cs_main);
};

/**
 * Gerenciador de múltiplos estados de corrente (útil para AssumeUTXO e snapshots)
 */
class ChainstateManager {
public:
    mutable RecursiveMutex m_chainman_mutex;
    std::vector<std::unique_ptr<Chainstate>> m_chainstates;

    /** Processa um novo bloco vindo da rede */
    bool ProcessNewBlock(const std::shared_ptr<const CBlock>& pblock, bool fForceProcessing, bool* fNewBlock) LOCKS_EXCLUDED(cs_main);

    /** Retorna a altura segura (Safety Floor) conforme sua regra de 10k blocos */
    int GetSafetyFloor() const;
};

/** * Mempool Acceptance Policy
 * Garante que transações no Mempool respeitem as taxas dinâmicas da MazeChain.
 */
bool AcceptToMemoryPool(Chainstate& active_chainstate, CTxMemPool& pool, const CTransactionRef& tx,
                       bool* pfMissingInputs, bool fOverrideFees = false) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/** Verifica se a Coinbase é válida e se as taxas foram para o Fundo de Reserva */
bool IsMazeCoinbaseValid(const CTransaction& tx, int nHeight, CAmount nFees, CAmount nTotalSupply, CAmount nReserveBalance);

#endif // MAZECHAIN_VALIDATION_H