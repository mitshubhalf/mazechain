// Copyright (c) 2026-present The MazeChain developers
// Distributed under the MIT software license.

#include "pow.h"
#include "arith_uint256.h"
#include "uint256.h"
#include "consensus/params.h"
#include <cassert>

// Definição segura de CBlockIndex
#ifndef CBLOCKINDEX_DEFINED
#define CBLOCKINDEX_DEFINED
class CBlockIndex {
public:
    const CBlockIndex* pprev;
    int nHeight;
    unsigned int nBits;
    unsigned int nTime;

    uint256 GetBlockHash() const;
    int64_t GetBlockTime() const { return (int64_t)nTime; }
};
#endif

/**
 * 🔥 FUNÇÃO AUXILIAR: CONVERTE "dificuldade lógica" → nBits real
 */
unsigned int DifficultyToBits(int difficulty, const Consensus::Params& params)
{
    arith_uint256 bnTarget = UintToArith256(params.powLimit);

    // Quanto maior a dificuldade → menor o target (mais zeros à esquerda)
    bnTarget >>= difficulty; 

    return bnTarget.GetCompact();
}

/**
 * MAZECHAIN - GetNextWorkRequired
 * Sincronizado com a nova regra: 5 fixa até 10k, Dinâmica 5 até 20k, Dinâmica 6+ após 20k.
 */
unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);

    int nHeight = pindexLast->nHeight + 1;

    // ============================================================
    // 🔥 NOVA REGRA DE CONSENSO (Sincronizada com Blockchain.cpp)
    // ============================================================

    // 1. BLOCO 1 AO 9.999: Dificuldade 5 Fixa
    if (nHeight > 0 && nHeight < 10000) {
        return DifficultyToBits(5, params);
    }

    // 2. BLOCO 10.000 AO 19.999: Dinâmica baseada em Dificuldade 5
    if (nHeight >= 10000 && nHeight < 20000) {
        // Se ainda não temos blocos suficientes para a janela dinâmica, retorna 5
        if (pindexLast->nHeight < 100) {
            return DifficultyToBits(5, params);
        }

        return CalculateNextWorkRequired(
            pindexLast,
            pindexLast->pprev->GetBlockTime(), // Ajustado conforme a lógica do seu motor
            params
        );
    }

    // 3. BLOCO 20.000 EM DIANTE: Dinâmica baseada em Dificuldade 6
    if (nHeight >= 20000) {
        // No exato momento do 20k, forçamos o aumento de carga
        return CalculateNextWorkRequired(
            pindexLast,
            pindexLast->pprev->GetBlockTime(),
            params
        );
    }

    // Default Fallback (Genesis ou erro)
    return DifficultyToBits(5, params);
}

/**
 * CalculateNextWorkRequired
 * Lógica de retargeting para ajustar a dificuldade conforme o Hashrate
 */
unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;

    // Limites de segurança para evitar saltos bruscos
    if (nActualTimespan < 1) nActualTimespan = 1;

    int64_t nTargetSpacing = params.nPowTargetSpacing;

    // Impede que a dificuldade mude mais de 4x para cima ou para baixo por ajuste
    if (nActualTimespan < nTargetSpacing / 4)
        nActualTimespan = nTargetSpacing / 4;

    if (nActualTimespan > nTargetSpacing * 4)
        nActualTimespan = nTargetSpacing * 4;

    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);

    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);

    bnNew *= nActualTimespan;
    bnNew /= nTargetSpacing;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

/**
 * CheckProofOfWorkImpl
 */
bool CheckProofOfWorkImpl(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Validações básicas de estrutura do target
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // O Hash deve ser MENOR ou IGUAL ao Target para ser válido
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}

/**
 * 🔥 CheckProofOfWork FINAL
 */
bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    return CheckProofOfWorkImpl(hash, nBits, params);
}