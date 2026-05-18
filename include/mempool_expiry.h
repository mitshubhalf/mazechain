#ifndef MEMPOOL_EXPIRY_H
#define MEMPOOL_EXPIRY_H

// ── MazeChain — Expiração de Mempool (TTL 24h) ───────────────────────────────
// Transações não confirmadas em mais de 24 horas são descartadas automaticamente.
// Impede acúmulo de lixo e ataques de spam com txs antigas.

#include <vector>
#include <string>
#include <ctime>
#include <algorithm>
#include "transaction.h"
#include "storage.h"

namespace MempoolExpiry {

    static const long MEMPOOL_TTL_SECONDS = 86400; // 24 horas

    // Retorna true se a transação ainda está dentro do prazo de validade
    inline bool isAlive(const Transaction& tx) {
        if (tx.signature == "coinbase") return true;
        if (tx.timestamp == 0) return true; // sem timestamp: assume válida (backward compat)
        long now = (long)std::time(nullptr);
        return (now - tx.timestamp) < MEMPOOL_TTL_SECONDS;
    }

    // Remove transações expiradas do arquivo de mempool
    // Retorna o número de transações removidas
    inline int purgeExpired(const std::string& mempoolPath) {
        std::vector<Transaction> all = Storage::loadMempool(mempoolPath);
        std::vector<Transaction> valid;
        int removed = 0;

        for (const auto& tx : all) {
            if (isAlive(tx)) {
                valid.push_back(tx);
            } else {
                removed++;
                std::cout << "🗑️  [MEMPOOL] Transação expirada removida: " << tx.id.substr(0, 16) << "..." << std::endl;
            }
        }

        if (removed > 0) {
            // Reescreve o mempool apenas com as válidas
            Storage::clearMempool(mempoolPath);
            for (const auto& tx : valid) {
                Storage::saveMempool(tx, mempoolPath);
            }
        }
        return removed;
    }

    // Filtra uma lista em memória (sem tocar no disco)
    inline std::vector<Transaction> filter(const std::vector<Transaction>& txs) {
        std::vector<Transaction> valid;
        for (const auto& tx : txs) {
            if (isAlive(tx)) valid.push_back(tx);
        }
        return valid;
    }
}

#endif
