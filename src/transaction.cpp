#include "../include/transaction.h"
#include "../include/crypto.h"
#include <sstream>
#include <ctime>
#include <iomanip>

// Construtor padrão
Transaction::Transaction() {
    id = "";
    signature = "";
    publicKey = "";
}

// Lógica para gerar o hash único da transação (ID)
// Essencial para o sistema de 64 bits e precisão double
std::string Transaction::calculateHash() const {
    std::stringstream ss;

    // 1. Incluímos todas as saídas (endereços e valores em Mits)
    // O uso de 'fixed' e 'setprecision(8)' é CRÍTICO para evitar que 
    // a notação científica (ex: 1e-05) quebre a consistência do hash.
    for (const auto& out : vout) {
        ss << out.address << std::fixed << std::setprecision(8) << out.amount;
    }

    // 2. Adicionamos a Chave Pública ou Seed do remetente
    ss << publicKey;

    // 3. Verificação de ID existente
    // Se a transação já tem um ID definitivo (carregado do banco de dados), 
    // não recalculamos para evitar divergências de histórico.
    if (!id.empty() && id.substr(0, 8) != "pending_") {
        return id;
    }

    // 4. Retorna o SHA-256 do conteúdo da transação
    return Crypto::sha256_util(ss.str());
}