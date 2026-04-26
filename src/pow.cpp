// Copyright (c) 2026 mits
// Copyright (c) present The MAZECHAIN developers
// Copyright (c) 2026-present The MazeChain developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>
#include <util/check.h>

/**
 * MAZECHAIN - GetNextWorkRequired
 * Ajuste de dificuldade contínuo para manter os 10 minutos estáveis.
 */
unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // MazeChain ajusta a dificuldade em TODOS os blocos para máxima segurança
    // (Diferente do Bitcoin que espera 2016 blocos)

    // 1. Pegar o tempo do bloco anterior ao anterior para calcular o delta
    const CBlockIndex* pindexFirst = pindexLast->pprev;
    if (pindexFirst == nullptr) return nProofOfWorkLimit;

    return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
}

/**
 * MAZECHAIN - CalculateNextWorkRequired
 * Implementa o ajuste dinâmico e o PISO de dificuldade.
 */
unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // 2. Calcular o tempo real levado para minerar o último bloco
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;

    // Proteção MazeChain: Impedir ajustes bruscos (max 4x ou min 0.25x)
    if (nActualTimespan < params.nPowTargetSpacing / 4)
        nActualTimespan = params.nPowTargetSpacing / 4;
    if (nActualTimespan > params.nPowTargetSpacing * 4)
        nActualTimespan = params.nPowTargetSpacing * 4;

    // 3. Cálculo do novo Alvo (Target)
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);

    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetSpacing;

    // 4. APLICAÇÃO DO PISO DE SEGURANÇA MAZECHAIN
    // Se o novo alvo for "mais fácil" que o limite da rede, volta pro limite.
    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

/**
 * MAZECHAIN - CheckProofOfWork
 * Validação fundamental: o hash do bloco é menor que o alvo exigido?
 */
bool CheckProofOfWorkImpl(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Verificações de sanidade do Target
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // VERIFICAÇÃO FINAL: O Hash minerado prova o esforço?
    // Se o hash (como número) for maior que o alvo, o trabalho é inválido.
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}

// CheckProofOfWork secundário para compatibilidade
bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    return CheckProofOfWorkImpl(hash, nBits, params);
}