// Copyright (c) 2026 mits
// Copyright (c) 2009-present mazechain Core developers
// Copyright (c) 2026-present The MazeChain developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/miner.h>

#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <common/args.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <logging.h>
#include <node/context.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <pow.h>
#include <primitives/transaction.h>
#include <util/moneystr.h>
#include <util/time.h>
#include <validation.h>

#include <algorithm>
#include <utility>
#include <numeric>

namespace node {

/**
 * MAZECHAIN - UpdateTime
 * Garante que o timestamp do bloco respeite os 10 minutos e as regras de derivação.
 */
int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev)
{
    int64_t nOldTime = pblock->nTime;
    int64_t nNewTime{std::max<int64_t>(pindexPrev->GetMedianTimePast() + 1, TicksSinceEpoch<std::chrono::seconds>(NodeClock::now()))};

    if (nOldTime < nNewTime) {
        pblock->nTime = nNewTime;
    }

    return nNewTime - nOldTime;
}

BlockAssembler::BlockAssembler(Chainstate& chainstate, const CTxMemPool* mempool, const Options& options)
    : chainparams{chainstate.m_chainman.GetParams()},
      m_mempool{options.use_mempool ? mempool : nullptr},
      m_chainstate{chainstate},
      m_options{options}
{
}

/**
 * MAZECHAIN - CreateNewBlock
 * Onde a mágica acontece: Monta o bloco, calcula taxas e separa o Fundo de Reserva.
 */
std::unique_ptr<CBlockTemplate> BlockAssembler::CreateNewBlock()
{
    resetBlock();

    pblocktemplate.reset(new CBlockTemplate());
    CBlock* const pblock = &pblocktemplate->block;

    // Adiciona espaço para a transação Coinbase (Recompensa + Taxas)
    pblock->vtx.emplace_back();

    LOCK(::cs_main);
    CBlockIndex* pindexPrev = m_chainstate.m_chain.Tip();
    nHeight = pindexPrev->nHeight + 1;

    pblock->nTime = TicksSinceEpoch<std::chrono::seconds>(NodeClock::now());
    m_lock_time_cutoff = pindexPrev->GetMedianTimePast();

    // 1. Incluir transações do Mempool (aqui elas já foram validadas com a taxa Maze de 1-7%)
    if (m_mempool) {
        LOCK(m_mempool->cs);
        addChunks(); 
    }

    // 2. CONSTRUÇÃO DA COINBASE DA MAZECHAIN
    CMutableTransaction coinbaseTx;
    coinbaseTx.vin.resize(1);
    coinbaseTx.vin[0].prevout.SetNull();
    coinbaseTx.vin[0].scriptSig = CScript() << nHeight << OP_0; // BIP34

    // Cálculo da Recompensa Fixa (400 MZ)
    CAmount nReward = GetBlockSubsidy(nHeight, chainparams.GetConsensus());

    /**
     * LOGICA DE DIVISÃO:
     * vout[0] -> Minerador (400 MZ Fixos)
     * vout[1] -> Fundo de Reserva (Total de nFees coletadas)
     */
    coinbaseTx.vout.resize(2);

    // Output 0: Minerador (Apenas a recompensa de bloco)
    coinbaseTx.vout[0].scriptPubKey = m_options.coinbase_output_script;
    coinbaseTx.vout[0].nValue = nReward;

    // Output 1: Fundo de Reserva (Recebe todas as taxas MZ acumuladas no bloco)
    // O endereço é puxado automaticamente do chainparams.cpp
    const std::string& reserveAddr = chainparams.GetConsensus().m_maze_reserve_fund_address;
    coinbaseTx.vout[1].scriptPubKey = GetScriptForDestination(DecodeDestination(reserveAddr));
    coinbaseTx.vout[1].nValue = nFees;

    pblock->vtx[0] = MakeTransactionRef(std::move(coinbaseTx));
    pblocktemplate->vTxFees[0] = -nFees;

    // 3. Finalização do Bloco
    pblock->hashPrevBlock = pindexPrev->GetBlockHash();
    UpdateTime(pblock, chainparams.GetConsensus(), pindexPrev);

    // Aplica o ajuste de dificuldade (Pisos Nível 4, 6 ou Dinâmico 10min)
    pblock->nBits = GetNextWorkRequired(pindexPrev, pblock, chainparams.GetConsensus());
    pblock->nNonce = 0;

    return std::move(pblocktemplate);
}

// ... (Restante das funções auxiliares de mempool permanecem similares ao Core para manter performance)

} // namespace node