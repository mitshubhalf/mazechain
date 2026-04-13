#include "../include/transaction.h"
#include "../include/crypto.h"
#include <sstream>

// Construtor opcional para inicialização rápida
Transaction::Transaction() {
    id = "";
    signature = "";
    publicKey = "";
}

// Lógica para gerar o hash único da transação
std::string Transaction::calculateHash() const {
    std::stringstream ss;
    for (const auto& out : vout) {
        ss << out.address << out.amount;
    }
    // Adiciona timestamp ou algo único se necessário
    return Crypto::sha256_util(ss.str());
}
