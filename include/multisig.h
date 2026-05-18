#ifndef MULTISIG_H
#define MULTISIG_H

// ── MazeChain — Multi-Assinatura M-de-N ──────────────────────────────────────
// Permite transações que requerem M assinaturas de um total de N signatários.
// Uso típico: 2-de-3 (exchange, backup de carteira, contratos entre partes)

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "crypto.h"
#include "transaction.h"

namespace Multisig {

    // Carteira multi-assinatura
    struct MultisigWallet {
        std::string address;          // Endereço MZms... gerado deterministicamente
        std::vector<std::string> signers; // Lista de endereços dos N signatários
        int required;                 // M — quantas assinaturas são obrigatórias
        int total;                    // N — total de signatários
    };

    // Transação multi-sig pendente (aguardando assinaturas)
    struct PendingMultisig {
        std::string txid;             // ID da transação a ser assinada
        std::string from;             // Carteira multisig de origem
        std::string to;               // Destino
        double amount;                // Valor
        std::vector<std::string> collected_sigs;  // Assinaturas coletadas
        std::vector<std::string> signed_by;        // Endereços que já assinaram
        int required;                 // Quantas precisamos
        long timestamp;               // Quando foi criada
    };

    // Cria o endereço multisig a partir dos signatários e threshold
    // address = "MZms" + sha256(sorted_signers_concat + "M")
    inline MultisigWallet createMultisigWallet(
        const std::vector<std::string>& signers, int required)
    {
        if (required < 1 || required > (int)signers.size()) {
            throw std::runtime_error("Threshold inválido: M deve ser entre 1 e N.");
        }

        std::vector<std::string> sorted = signers;
        std::sort(sorted.begin(), sorted.end());

        std::string data;
        for (const auto& s : sorted) data += s;
        data += std::to_string(required);
        data += "MAZE_MULTISIG_V1";

        std::string hash = Crypto::sha256_util(data);

        MultisigWallet w;
        w.address  = "MZms" + hash.substr(0, 28);
        w.signers  = sorted;
        w.required = required;
        w.total    = (int)sorted.size();
        return w;
    }

    // Verifica se um endereço é signatário autorizado de uma carteira multisig
    inline bool isSigner(const MultisigWallet& wallet, const std::string& address) {
        return std::find(wallet.signers.begin(), wallet.signers.end(), address)
               != wallet.signers.end();
    }

    // Salva transação multisig pendente em arquivo simples (data/multisig_pending.dat)
    inline void savePending(const PendingMultisig& p, const std::string& path) {
        std::ofstream f(path, std::ios::app);
        if (!f.is_open()) return;
        f << p.txid << "|"
          << p.from << "|"
          << p.to   << "|"
          << std::fixed << p.amount << "|"
          << p.required << "|"
          << p.timestamp;
        for (const auto& sig : p.collected_sigs) f << "|SIG:" << sig;
        for (const auto& addr : p.signed_by)     f << "|SIGNER:" << addr;
        f << "\n";
        f.close();
    }

    // Carrega todas as transações multisig pendentes
    inline std::vector<PendingMultisig> loadPending(const std::string& path) {
        std::vector<PendingMultisig> result;
        std::ifstream f(path);
        if (!f.is_open()) return result;

        std::string line;
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            std::istringstream ss(line);
            std::string token;
            PendingMultisig p;

            std::vector<std::string> tokens;
            while (std::getline(ss, token, '|')) tokens.push_back(token);

            if (tokens.size() < 6) continue;
            p.txid      = tokens[0];
            p.from      = tokens[1];
            p.to        = tokens[2];
            try { p.amount = std::stod(tokens[3]); } catch (...) { p.amount = 0; }
            try { p.required = std::stoi(tokens[4]); } catch (...) { p.required = 1; }
            try { p.timestamp = std::stol(tokens[5]); } catch (...) { p.timestamp = 0; }

            for (size_t i = 6; i < tokens.size(); i++) {
                if (tokens[i].substr(0, 4) == "SIG:") {
                    p.collected_sigs.push_back(tokens[i].substr(4));
                } else if (tokens[i].substr(0, 7) == "SIGNER:") {
                    p.signed_by.push_back(tokens[i].substr(7));
                }
            }
            result.push_back(p);
        }
        return result;
    }

    // Remove um txid das pendências (após broadcast ou expiração)
    inline void removePending(const std::string& txid, const std::string& path) {
        auto all = loadPending(path);
        // Reescreve o arquivo sem a transação removida
        std::ofstream f(path, std::ios::trunc);
        for (const auto& p : all) {
            if (p.txid == txid) continue;
            f << p.txid << "|" << p.from << "|" << p.to << "|"
              << std::fixed << p.amount << "|" << p.required << "|" << p.timestamp;
            for (const auto& sig : p.collected_sigs) f << "|SIG:" << sig;
            for (const auto& addr : p.signed_by)     f << "|SIGNER:" << addr;
            f << "\n";
        }
    }

    // Adiciona uma assinatura parcial a uma transação pendente
    // Retorna true se agora temos M assinaturas e a tx pode ser transmitida
    inline bool addSignature(
        const std::string& txid,
        const std::string& signer_address,
        const std::string& signature,
        const std::string& path)
    {
        auto all = loadPending(path);
        bool ready = false;

        for (auto& p : all) {
            if (p.txid != txid) continue;

            // Verifica se já assinou
            if (std::find(p.signed_by.begin(), p.signed_by.end(), signer_address)
                != p.signed_by.end()) {
                throw std::runtime_error("Este signatário já assinou esta transação.");
            }

            p.collected_sigs.push_back(signature);
            p.signed_by.push_back(signer_address);

            if ((int)p.collected_sigs.size() >= p.required) {
                ready = true;
            }
            break;
        }

        // Reescreve arquivo com a nova assinatura
        std::ofstream f(path, std::ios::trunc);
        for (const auto& p : all) {
            f << p.txid << "|" << p.from << "|" << p.to << "|"
              << std::fixed << p.amount << "|" << p.required << "|" << p.timestamp;
            for (const auto& sig : p.collected_sigs) f << "|SIG:" << sig;
            for (const auto& addr : p.signed_by)     f << "|SIGNER:" << addr;
            f << "\n";
        }

        return ready;
    }

} // namespace Multisig

#endif
