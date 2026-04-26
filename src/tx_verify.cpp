// Copyright (c) 2026-present The mazechain Core developers
// Copyright (c) 2026-present The MazeChain developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/tx_verify.h>
#include <chain.h>
#include <coins.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <primitives/transaction.h>
#include <util/check.h>
#include <util/moneystr.h>

/**
 * MAZECHAIN DYNAMIC FEE CALCULATOR
 * Calcula a taxa mínima obrigatória com base na altura do bloco.
 * 1% inicial, subindo conforme a escassez.
 */
CAmount GetMazeMinimumFee(CAmount nValueIn, int nHeight) {
    double feePercentage = 0.01; // 1% padrão

    if (nHeight > 10000) feePercentage = 0.03; // 3% após 1º halving
    if (nHeight > 20000) feePercentage = 0.05; // 5% após 2º halving
    if (nHeight > 50000) feePercentage = 0.07; // 7% teto máximo

    return static_cast<CAmount>(nValueIn * feePercentage);
}

/**
 * CONSENSUS: CheckTxInputs (Versão MazeChain)
 * Verifica se os inputs existem, se a coinbase maturou (50 blocos)
 * e se a TAXA DINÂMICA foi respeitada.
 */
bool Consensus::CheckTxInputs(const CTransaction& tx, TxValidationState& state, const CCoinsViewCache& inputs, int nSpendHeight, CAmount& txfee)
{
    // 1. Verificação de Existência: Os inputs estão no UTXO Set?
    if (!inputs.HaveInputs(tx)) {
        return state.Invalid(TxValidationResult::TX_MISSING_INPUTS, "bad-txns-inputs-missingorspent");
    }

    CAmount nValueIn = 0;
    for (unsigned int i = 0; i < tx.vin.size(); ++i) {
        const COutPoint &prevout = tx.vin[i].prevout;
        const Coin& coin = inputs.AccessCoin(prevout);
        assert(!coin.IsSpent());

        // 2. MATURAÇÃO MAZECHAIN: 50 blocos para Coinbase
        if (coin.IsCoinBase() && nSpendHeight - coin.nHeight < 50) {
            return state.Invalid(TxValidationResult::TX_PREMATURE_SPEND, "bad-txns-premature-spend-of-coinbase",
                strprintf("MazeChain: Moedas imaturas. Aguarde %d blocos", 50 - (nSpendHeight - coin.nHeight)));
        }

        nValueIn += coin.out.nValue;
        if (!MoneyRange(coin.out.nValue) || !MoneyRange(nValueIn)) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-inputvalues-outofrange");
        }
    }

    const CAmount value_out = tx.GetValueOut();

    // 3. Verificação de Saldo: Não se pode gastar mais do que tem
    if (nValueIn < value_out) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-in-belowout",
            strprintf("Entrada (%s) menor que Saída (%s)", FormatMoney(nValueIn), FormatMoney(value_out)));
    }

    // 4. CÁLCULO DA TAXA DINÂMICA
    const CAmount txfee_aux = nValueIn - value_out;
    CAmount nMinRequiredFee = GetMazeMinimumFee(nValueIn, nSpendHeight);

    if (txfee_aux < nMinRequiredFee) {
        return state.Invalid(TxValidationResult::TX_LOW_FEE, "bad-txns-fee-too-low",
            strprintf("MazeChain: Taxa insuficiente. Mínimo exigido: %s (Sua taxa: %s)", 
            FormatMoney(nMinRequiredFee), FormatMoney(txfee_aux)));
    }

    txfee = txfee_aux;
    return true;
}

// ... (Manutenção das funções IsFinalTx, CalculateSequenceLocks e SigOps conforme o original)
// Essas funções garantem que a transação respeite o tempo de bloqueio e não sobrecarregue o processador.