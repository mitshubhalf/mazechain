#include "mempool_limit.h"
#include <algorithm>

// Use o namespace que você definiu no .h
namespace MempoolLimit {

    // Mude o nome de 'Prune' para 'TrimMempool' para casar com o .h
    void TrimMempool(std::vector<Transaction>& txs) {
        // Use a constante MAX_TX_COUNT que você definiu no .h
        if (txs.size() <= MAX_TX_COUNT) return;

        // Ordena para manter as que pagam mais taxas
        std::sort(txs.begin(), txs.end(), [](const Transaction& a, const Transaction& b) {
            return a.fee > b.fee;
        });

        // Remove o excesso
        txs.erase(txs.begin() + MAX_TX_COUNT, txs.end());
    }

    // Não esqueça de implementar a outra função que está no .h
    bool CanAcceptTransaction(int currentCount) {
        return currentCount < MAX_TX_COUNT;
    }
}