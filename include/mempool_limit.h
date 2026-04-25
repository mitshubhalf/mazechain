#ifndef MEMPOOL_LIMIT_H
#define MEMPOOL_LIMIT_H

#include <vector>
#include "transaction.h"

namespace MempoolLimit {
    // Limite sugerido: 50MB (baseado no tamanho médio de TX em memória)
    const size_t MAX_MEMPOOL_SIZE_MB = 50; 

    // Máximo de 5000 transações aguardando no arquivo mempool.dat
    const int MAX_TX_COUNT = 5000;          

    // Verifica se a mempool ainda aguenta novas transações
    bool CanAcceptTransaction(int currentCount);

    // Organiza a mempool para expulsar as mais baratas (menor taxa) se estiver cheia
    void TrimMempool(std::vector<Transaction>& vtx);
}

#endif