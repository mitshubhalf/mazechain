#include "../include/mempool_limit.h"
#include <algorithm>

namespace MempoolLimit {
    bool CanAcceptTransaction(int currentCount) {
        return currentCount < MAX_TX_COUNT;
    }

    void TrimMempool(std::vector<Transaction>& vtx) {
        if (vtx.size() <= MAX_TX_COUNT) return;

        // Ordena: as que pagam mais taxas ficam primeiro (considerando 1% do valor)
        std::sort(vtx.begin(), vtx.end(), [](const Transaction& a, const Transaction& b) {
            return a.vout[0].amount > b.vout[0].amount; 
        });

        // Remove o excesso (as menores/mais baratas)
        vtx.resize(MAX_TX_COUNT);
    }
}
