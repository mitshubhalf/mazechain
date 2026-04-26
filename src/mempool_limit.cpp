#include "mempool_limit.h"
#include <algorithm>
#include <vector>

namespace MempoolLimit {

    // Função para calcular a taxa de uma transação dinamicamente
    // Já que o compilador disse que 'fee' não existe na struct
    double GetTransactionFee(const Transaction& tx) {
        double input = 0;
        double output = 0;
        for (const auto& out : tx.vout) {
            if (out.amount < 0) input += std::abs(out.amount);
            else output += out.amount;
        }
        return input - output;
    }

    void TrimMempool(std::vector<Transaction>& txs) {
        if (txs.size() <= MAX_TX_COUNT) return;

        // Ordena para manter as que pagam mais taxas (calculando a taxa real)
        std::sort(txs.begin(), txs.end(), [](const Transaction& a, const Transaction& b) {
            return GetTransactionFee(a) > GetTransactionFee(b);
        });

        // Remove as transações excedentes (as de menor taxa)
        txs.erase(txs.begin() + MAX_TX_COUNT, txs.end());
    }

    bool CanAcceptTransaction(int currentCount) {
        return currentCount < MAX_TX_COUNT;
    }
}