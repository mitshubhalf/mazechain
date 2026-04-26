// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Copyright (c) 2026-present The MazeChain developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MAZECHAIN_POW_H
#define MAZECHAIN_POW_H

#include <consensus/params.h>
#include <optional>
#include <cstdint>

class CBlockHeader;
class CBlockIndex;
class uint256;
class arith_uint256;

/**
 * MazeChain - Proof of Work Interface
 */

/**
 * Converte o valor compacto 'nBits' para o alvo (target) de 256 bits.
 * Verifica se o alvo está dentro dos limites de segurança da MazeChain.
 */
std::optional<arith_uint256> DeriveTarget(unsigned int nBits, uint256 pow_limit);

/**
 * Calcula a dificuldade necessária para o PRÓXIMO bloco.
 * Na MazeChain, isso é chamado a cada bloco para garantir o tempo de 10 minutos.
 */
unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params);

/**
 * Lógica central do cálculo de ajuste.
 * Baseia-se no tempo real decorrido vs. o tempo alvo (TargetSpacing).
 */
unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params);

/** * Verifica se o Hash de um bloco minerado atende aos requisitos de dificuldade (nBits).
 * É o "Cadeado" que valida o trabalho do minerador.
 */
bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params);
bool CheckProofOfWorkImpl(uint256 hash, unsigned int nBits, const Consensus::Params& params);

/**
 * Verifica se a transição de dificuldade entre o bloco antigo e o novo é válida.
 * Na MazeChain, como o ajuste é a cada bloco, essa função garante que a 
 * dificuldade não varie mais que 4x (para cima ou para baixo) por bloco,
 * evitando instabilidade na rede.
 */
bool PermittedDifficultyTransition(const Consensus::Params& params, int64_t height, uint32_t old_nbits, uint32_t new_nbits);

#endif // MAZECHAIN_POW_H