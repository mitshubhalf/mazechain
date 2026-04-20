#ifndef MEMPOOL_LIMIT_H
#define MEMPOOL_LIMIT_H

#include <vector>
#include "transaction.h"

namespace MempoolLimit {
    const size_t MAX_MEMPOOL_SIZE_MB = 50; // Limite de 50 Megas
    const int MAX_TX_COUNT = 5000;         // Máximo de 5000 transações esperando

    // Verifica se a mempool ainda aguenta novas transações
    bool CanAcceptTransaction(int currentCount);
    
    // Organiza a mempool para expulsar as mais baratas se estiver cheia
    void TrimMempool(std::vector<Transaction>& vtx);
}

#endif
