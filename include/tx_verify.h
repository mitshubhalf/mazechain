// Copyright (c) 2017-present The Bitcoin Core developers
// Copyright (c) 2026-present The MazeChain developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MAZECHAIN_CONSENSUS_TX_VERIFY_H
#define MAZECHAIN_CONSENSUS_TX_VERIFY_H

#include <consensus/amount.h>
#include <script/verify_flags.h>

#include <cstdint>
#include <vector>

class CBlockIndex;
class CCoinsViewCache;
class CTransaction;
class TxValidationState;

/** * MAZECHAIN - CONSENSUS TX VERIFICATION
 * Define as regras de auditoria para cada transação na rede.
 */

namespace Consensus {
/**
 * CheckTxInputs: O coração da validação de saldos e taxas.
 * - Verifica se os inputs existem e são maduros (50 blocos para mineradores).
 * - Garante que a Taxa Dinâmica (1% a 7%) foi paga.
 * - @param[out] txfee Retorna a taxa calculada para ser enviada ao Fundo de Reserva.
 */
[[nodiscard]] bool CheckTxInputs(const CTransaction& tx, TxValidationState& state, const CCoinsViewCache& inputs, int nSpendHeight, CAmount& txfee);
} // namespace Consensus

/** * GetMazeMinimumFee: 
 * Função auxiliar para calcular o piso da taxa com base na altura da chain.
 */
CAmount GetMazeMinimumFee(CAmount nValueIn, int nHeight);

/** Auxiliares de SigOps (Proteção contra spam de scripts pesados) */

/** Calcula o custo de operações de assinatura (padrão industrial contra ataques DoS) */
int64_t GetTransactionSigOpCost(const CTransaction& tx, const CCoinsViewCache& inputs, script_verify_flags flags);

/** * Finalidade e Bloqueio de Tempo (LockTime)
 * Garante que transações com "nLockTime" não sejam aceitas antes da hora.
 */
bool IsFinalTx(const CTransaction &tx, int nBlockHeight, int64_t nBlockTime);

/** Lógica de BIP 68 (Sequence Locks) adaptada para a MazeChain */
std::pair<int, int64_t> CalculateSequenceLocks(const CTransaction &tx, int flags, std::vector<int>& prevHeights, const CBlockIndex& block);
bool EvaluateSequenceLocks(const CBlockIndex& block, std::pair<int, int64_t> lockPair);
bool SequenceLocks(const CTransaction &tx, int flags, std::vector<int>& prevHeights, const CBlockIndex& block);

#endif // MAZECHAIN_CONSENSUS_TX_VERIFY_H