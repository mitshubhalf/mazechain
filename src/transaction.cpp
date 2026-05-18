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
    sender = "";
    receiver = "";
    amount = 0.0;
    timestamp = 0;
}

// 🔥 NOVO CONSTRUTOR (Necessário para o comando 'send' e API)
// Mantém a compatibilidade com o sistema de 64 bits e precisão double
Transaction::Transaction(std::string s, std::string r, double a) {
    sender = s;
    receiver = r;
    amount = a;
    timestamp = std::time(nullptr);
    publicKey = ""; // Pode ser preenchida posteriormente se necessário

    // Geramos o ID inicial baseado nos dados da transferência
    id = calculateHash();
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

    // Se vout estiver vazio (como em transações simples de send), usamos os campos base
    if (vout.empty()) {
        ss << sender << receiver << std::fixed << std::setprecision(8) << amount << timestamp;
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
    // Ajustado para usar o seu wrapper Crypto::sha256_util
    return Crypto::sha256_util(ss.str());
}

// ── Assinatura Digital ────────────────────────────────────────────────────────
void Transaction::sign(std::string privateKey) {
    if (privateKey.empty()) {
        signature = "unsigned";
        return;
    }
    // A assinatura é gerada SOBRE o txid (que não inclui a assinatura)
    // garantindo anti-maleabilidade: alterar a assinatura não muda o txid.
    signature = Crypto::sha256_util(id + privateKey);
}

// ── Anti-Maleabilidade: ID canônico vs Witness ID ────────────────────────────
// txid (id)     = hash(vout + publicKey) — NÃO inclui assinatura
// witnessId()   = hash(id + signature)  — inclui assinatura, como wtxid do SegWit
//
// Isso garante que terceiros não podem alterar a assinatura e mudar o txid,
// o que é o ataque de maleabilidade de transações do Bitcoin pré-SegWit.
std::string Transaction::witnessId() const {
    // wtxid = hash(txid + signature) — inclui o "testemunho" (witness data)
    return Crypto::sha256_util(id + signature + publicKey);
}

// Verifica se a assinatura é válida para esta transação
// Usa o txid (sem assinatura) como dado a ser verificado — padrão anti-maleável
bool Transaction::verifySignature() const {
    if (signature == "coinbase" || signature == "unsigned") return true;
    if (id.empty() || publicKey.empty()) return false;
    // Verifica que a assinatura corresponde ao txid assinado com a chave privada
    // (compatível com o esquema HMAC usado no Wallet::sign)
    return Crypto::verify_signature(id, signature, publicKey);
}