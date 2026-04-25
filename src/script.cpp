#include "../include/script.h"
#include "../include/crypto.h"

/**
 * A verificação é o coração do sistema: 
 * Ela garante que a transação não foi alterada e que o remetente é legítimo.
 */
bool Script::verify(const std::string& publicKey, const std::string& signature, const std::string& message) {
    // Chama a criptografia de Curva Elíptica para validar a assinatura
    return Crypto::verify_signature(message, signature, publicKey);
}

/**
 * O script de bloqueio é como um cofre que diz:
 * "Para abrir este cofre, você precisa duplicar sua chave pública, 
 * transformá-la em hash, verificar se bate com este endereço e assinar digitalmente."
 */
std::string Script::create_locking_script(const std::string& address) {
    // Formato inspirado no Bitcoin: OP_DUP OP_HASH160 <Endereço> OP_EQUALVERIFY OP_CHECKSIG
    // Isso garante que a moeda está "presa" ao endereço MZ destino.
    return "OP_DUP OP_HASH160 " + address + " OP_EQUALVERIFY OP_CHECKSIG";
}
