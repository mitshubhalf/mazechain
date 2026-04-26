// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Copyright (c) 2026-present The MazeChain developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MAZECHAIN_CHAINPARAMS_H
#define MAZECHAIN_CHAINPARAMS_H

#include <kernel/chainparams.h> // IWYU pragma: export
#include <util/chaintype.h>

#include <memory>

class ArgsManager;

/**
 * MAZECHAIN - CHAIN PARAMETERS INTERFACE
 * Esta interface expõe as configurações globais da rede para o código.
 */

/**
 * Cria e retorna um ponteiro único para as configurações da MazeChain (Main, Testnet ou Regtest).
 * Aqui é onde as janelas de dificuldade (0-5k, 5k-10k) são instanciadas.
 */
std::unique_ptr<const CChainParams> CreateChainParams(const ArgsManager& args, ChainType chain);

/**
 * Retorna os parâmetros da rede selecionada no momento.
 * Usado em todo o código (ex: Params().GetConsensus()) para verificar regras de taxa e PoW.
 */
const CChainParams &Params();

/**
 * Define qual rede a MazeChain está rodando (geralmente chamada no início do maze-node).
 */
void SelectParams(ChainType chain);

/**
 * Helper para verificar se a rede atual é a Mainnet da MazeChain.
 */
inline bool IsMazeMainNet() {
    return Params().GetChainType() == ChainType::MAIN;
}

#endif // MAZECHAIN_CHAINPARAMS_H