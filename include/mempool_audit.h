#ifndef MEMPOOL_AUDIT_H
#define MEMPOOL_AUDIT_H

#include <string>
#include <vector>
#include <cmath>
#include "blockchain.h"
#include "storage.h"

namespace MempoolAudit {

    // Calcula quanto o endereço já comprometeu no mempool (Valor + Taxas)
    double GetPendingSpent(const std::string& address, const std::string& mempoolPath) {
        double pending = 0.0;

        // Carrega as transações que estão aguardando no arquivo
        std::vector<Transaction> mempoolTxs = Storage::loadMempool(mempoolPath);

        for (const auto& tx : mempoolTxs) {
            // No seu sistema, vout[1] é sempre o débito total da conta (valor negativo)
            // vout[0] é o que o destinatário recebe.
            if (tx.vout.size() >= 2 && tx.vout[1].address == address) {
                // Somamos o valor absoluto do vout[1], que já inclui (Valor Enviado + Taxa)
                pending += std::abs(tx.vout[1].amount);
            }
        }
        return pending;
    }

    // Verifica se o saldo é suficiente considerando o que já está no mempool
    bool IsTransactionAllowed(Blockchain& chain, const std::string& sender, double amountPlusFee, const std::string& mempoolPath) {
        double realBalance = chain.getBalance(sender);
        double alreadyPending = GetPendingSpent(sender, mempoolPath);

        // Se o saldo real for maior ou igual ao que já gastei no mempool + o que quero gastar agora
        if (realBalance >= (alreadyPending + amountPlusFee)) {
            return true; // Autorizado
        }

        return false; // Saldo insuficiente (evita "double spend" no mempool)
    }
}

#endif