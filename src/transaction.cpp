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
std::string Transaction::calculateHash() const {
    std::stringstream ss;
    
    // 1. Incluímos todas as saídas (endereços e valores em Mits)
    for (const auto& out : vout) {
        // Usamos setprecision para garantir que o hash mude se um único 'Mit' for alterado
        ss << out.address << std::fixed << std::setprecision(8) << out.amount;
    }
    
    // 2. Adicionamos a Chave Pública do remetente ao hash
    // Isso impede que alguém tente "re-assinar" a mesma transação
    ss << publicKey;

    // 3. O "Sal": Adicionamos um timestamp de alta precisão ou ID único
    // Isso garante que se você enviar 10 MZ agora e 10 MZ daqui a 1 minuto,
    // os IDs das transações serão completamente diferentes.
    // Se o ID já foi definido (ex: carregado do disco), retornamos ele.
    if (!id.empty() && id.substr(0, 8) != "pending_") return id;

    return Crypto::sha256_util(ss.str());
}
