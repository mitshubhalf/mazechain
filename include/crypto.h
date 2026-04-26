#ifndef CRYPTO_H
#define CRYPTO_H

#include <string>
#include <vector>

// Forward declaration para evitar inclusão circular 
struct Transaction; 

namespace Crypto {

    // --- Estruturas de Dados ---

    // Armazena o par de chaves derivadas da Seed
    struct KeyPair {
        std::string private_key;
        std::string public_key;
    };

    // --- Funções de Hashing (O Coração da Blockchain) ---

    // Retorna o hash SHA256 simples
    std::string sha256_util(std::string str);

    // Retorna o SHA256 Duplo (SHA256(SHA256(data))) - Padrão Bitcoin
    // Essencial para IDs de transação e Hashes de Bloco
    std::string sha256d(std::string str);

    // Constrói a árvore de Merkle utilizando SHA256 Duplo
    // Garante a integridade imutável de todas as transações dentro de um bloco
    std::string calculateMerkleRoot(const std::vector<Transaction>& txs);

    // --- Funções de Criptografia Assimétrica (Segurança da Carteira) ---

    // Gera um par de chaves determinístico a partir de uma Seed
    // A chave privada é usada para assinar; a pública gera o endereço MZ
    KeyPair generate_keys_from_seed(const std::string& seed);

    // Cria uma assinatura digital SHA256d para os dados da transação
    // Isso garante que apenas o dono da chave privada possa gastar as moedas
    std::string sign_data(const std::string& data, const std::string& private_key);

    // Verifica a validade de uma assinatura digital
    // Permite que mineradores confirmem a autenticidade sem possuir a chave privada
    bool verify_signature(const std::string& data, const std::string& signature, const std::string& public_key);

    // --- MELHORIA: Funções de Criptografia de Wallet (AES-256-CBC) ---
    // Estas funções são necessárias para a API gerir seeds protegidas por password

    // Encripta dados sensíveis (como a Seed) usando uma password
    std::string encrypt_data(const std::string& plaintext, const std::string& password);

    // Desencripta os dados (como a Seed encriptada) usando a password
    std::string decrypt_data(const std::string& ciphertext_hex, const std::string& password);

} // namespace Crypto

#endif