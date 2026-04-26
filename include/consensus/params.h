// Copyright (c) 2026 mits
// Copyright (c) 2026 The mazechain developers
// Copyright (c) 2026-present The MazeChain developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MAZECHAIN_CONSENSUS_PARAMS_H
#define MAZECHAIN_CONSENSUS_PARAMS_H

#include <script/verify_flags.h>
#include <uint256.h>

#include <array>
#include <chrono>
#include <limits>
#include <map>
#include <vector>
#include <string>

namespace Consensus {

/**
 * MAZECHAIN - Parâmetros de Consenso Dinâmico
 */
struct Params {
    uint256 hashGenesisBlock;
    int nSubsidyHalvingInterval; // Para você, 10.000 blocos

    /** Regras de ativação hardcoded (MazeChain inicia com todas ativas) */
    int BIP34Height;
    int BIP65Height;
    int BIP66Height;
    int CSVHeight;
    int SegwitHeight;

    /** * MAZECHAIN - PISOS DE DIFICULDADE (Sua Regra Customizada)
     */
    int nHeightLevel4; // 5.000
    int nHeightLevel6; // 10.000

    /** Proof of work parameters */
    uint256 powLimit;
    bool fPowAllowMinDifficultyBlocks;
    bool fPowNoRetargeting;
    int64_t nPowTargetSpacing;  // 600 (10 minutos)
    int64_t nPowTargetTimespan; // 600 (Ajuste a cada bloco)

    /** * MAZECHAIN - RESERVA E TAXAS
     * Armazena o endereço que receberá a fatia das taxas dinâmicas.
     */
    std::string m_maze_reserve_fund_address;

    /** Funções auxiliares de tempo */
    std::chrono::seconds PowTargetSpacing() const
    {
        return std::chrono::seconds{nPowTargetSpacing};
    }

    // Na MazeChain, o intervalo é 1 (nPowTargetTimespan / nPowTargetSpacing = 1)
    int64_t DifficultyAdjustmentInterval() const { return nPowTargetTimespan / nPowTargetSpacing; }

    /** Segurança de Chain */
    uint256 nMinimumChainWork;
    uint256 defaultAssumeValid;

    /** BIP94 e proteções contra ataques de tempo */
    bool enforce_BIP94;
};

} // namespace Consensus

#endif // MAZECHAIN_CONSENSUS_PARAMS_H