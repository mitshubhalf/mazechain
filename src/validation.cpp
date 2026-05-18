/**
 * MAZECHAIN CORE - VALIDATION ENGINE (INDUSTRIAL GRADE)
 * Copyright (c) 2026-present The MazeChain developers
 */

#include <validation.h>
#include <consensus/params.h>
#include <consensus/amount.h>
#include <consensus/validation.h>
#include <chainparams.h>
#include <checkpoints.h>
#include <logging.h>
#include <util/moneystr.h>
#include <util/strencodings.h>

/**
 * SUBSÍDIO OFICIAL MAZECHAIN (NOVA REGRA: DECAIMENTO 8% PARA TOTAL 20M)
 * Implementação exata das regras de emissão e transição para o Fundo de Reserva.
 */
CAmount GetMazeBlockSubsidy(int nHeight, const CAmount& nTotalSupply, const CAmount& nReserveBalance)
{
    const CAmount MAX_SUPPLY_LIMIT = 20000000 * COIN;
    const int interval = 10000;
    int halving_count = nHeight / interval;

    // FASE 5: RECURSÃO E VIDA ETERNA (Pós-20M MZ ou Pós-Era 64)
    // O minerador para de criar moedas e passa a "pescar" do fundo acumulado pelas taxas.
    if (nTotalSupply >= MAX_SUPPLY_LIMIT || halving_count >= 64) {
        if (nReserveBalance > 10000) { // Mínimo de 0.00010000 MZ no fundo para saque
            // Retira 0.01% do fundo para o minerador (Sustentabilidade Infinita)
            return nReserveBalance / 10000; 
        }
        return 1; // Subsídio mínimo absoluto (1 mit)
    }

    // Protocol v4.0: 100 × (0.95205055 ^ halving_count)
    // Decaimento: ~4.794945% por Era — 64 Halvings — Max Supply 20M
    double reward = 100.0 * std::pow(0.95205055, halving_count);

    CAmount nSubsidy = static_cast<CAmount>(reward * COIN);

    // Proteção de teto: Garante que o subsídio não ultrapasse o que falta para 20M
    if (nTotalSupply + nSubsidy > MAX_SUPPLY_LIMIT) {
        nSubsidy = MAX_SUPPLY_LIMIT - nTotalSupply;
    }

    return (nSubsidy < 1) ? 1 : nSubsidy;
}

/**
 * CÁLCULO DE TAXA DINÂMICA (REGRA 1% - 7%)
 */
double GetRequiredFeePercentage(int nHeight, const CAmount& nTotalSupply) {
    const CAmount TWENTY_MILLION = 20000000 * COIN;
    int halving_count = nHeight / 10000;

    // Escalonamento conforme a maturidade da rede
    if (nHeight <= 10000) return 0.01;  // Era 0: 1%
    if (nHeight <= 20000) return 0.02;  // Era 1: 2%
    if (nHeight <= 30000) return 0.025; // Era 2: 2.5%

    // Fase de Escassez: Sobe para 5% conforme o subsídio de bloco diminui
    if (halving_count >= 45 && nTotalSupply < TWENTY_MILLION) return 0.05;

    // Sustentabilidade Total: 7% fixo quando o supply de 20M for atingido (Era 64+)
    if (nTotalSupply >= TWENTY_MILLION || halving_count >= 64) return 0.07;

    return 0.03; // Taxa padrão para eras intermediárias
}

/**
 * EXECUTE MAZE VALIDATION
 * Auditoria rigorosa de cada bloco antes da aceitação na corrente.
 */
bool Chainstate::ExecuteMazeValidation(const CBlock& block, BlockValidationState& state, CBlockIndex* pindex)
{
    AssertLockHeld(cs_main);

    // 1. Verificação de SafetyFloor (Checkpoints e Eras)
    // Impede forks profundos fora da Era atual (janelas de 10.000 blocos)
    int nSafetyFloor = std::max(Checkpoints::GetLastCheckpointHeight(), (int)(pindex->nHeight / 10000) * 10000);
    if (pindex->nHeight < nSafetyFloor) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-fork-safety-floor");
    }

    CAmount nFees = 0;
    CAmount nChainSupply = m_chainman.ActiveChain().Tip()->nChainSupply;

    // 2. Auditoria de Transações e Taxas Dinâmicas
    double reqPercent = GetRequiredFeePercentage(pindex->nHeight, nChainSupply);

    for (const auto& tx : block.vtx) {
        if (tx->IsCoinBase()) continue; 

        // Validação de Assinaturas Criptográficas
        if (!Consensus::VerifyMazeSignatures(*tx, state)) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-tx-signature");
        }

        // Auditoria de Taxa: nValueIn (UTXOs gastos) - nValueOut (destinos)
        CAmount txIn = m_chainman.ActiveChainstate().GetValueIn(*tx);
        CAmount txOut = tx->GetValueOut();
        CAmount txFee = txIn - txOut;

        // Verifica se a taxa paga atende à porcentagem dinâmica da rede
        if (txFee < static_cast<CAmount>(txOut * reqPercent)) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "insufficient-dynamic-fee",
                strprintf("Taxa insuficiente. Altura %d exige %.1f%%", pindex->nHeight, reqPercent * 100));
        }

        nFees += txFee;
    }

    // 3. Validação da Coinbase (Subsídio + Destino das Taxas)
    CAmount nReserveBalance = m_chainman.GetReserveFundBalance();
    CAmount expectedSubsidy = GetMazeBlockSubsidy(pindex->nHeight, nChainSupply, nReserveBalance);

    const CTransaction& coinbase = *block.vtx[0];
    CAmount nMinerReward = coinbase.vout[0].nValue;

    // O minerador não pode dar a si mesmo mais do que o permitido pelo consenso
    if (nMinerReward > expectedSubsidy) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cb-subsidy-limit");
    }

    // 4. Verificação do Fundo de Reserva (MZ_SYSTEM_RESERVE_FUND)
    // Se houve taxas no bloco, elas DEVEM ser enviadas para o vout[1] da coinbase (Script da Reserva)
    if (nFees > 0) {
        if (coinbase.vout.size() < 2) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-reserve-fund-missing");
        }

        // Valida o endereço de destino (ScriptPubKey do Fundo de Reserva)
        if (coinbase.vout[1].scriptPubKey != RESERVE_FUND_SCRIPT) {
             return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-reserve-fund-diversion");
        }

        // Valida se o valor enviado para a reserva é exatamente a soma das taxas coletadas
        if (coinbase.vout[1].nValue != nFees) {
             return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-reserve-fee-amount");
        }
    }

    return true;
}