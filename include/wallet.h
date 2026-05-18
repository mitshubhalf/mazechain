#ifndef WALLET_H
#define WALLET_H

#include <string>
#include <vector>
#include "blockchain.h"
#include "hd_wallet.h"  // BIP-32 HD Wallet

/**
 * MAZECHAIN - Wallet System
 * Gerencia chaves privadas, endereços e persistência de dados.
 */
class Wallet {
public:
    // Atributos Públicos (Acesso direto para simplificar o main.cpp)
    std::string address;  // Endereço público (ex: MZ...)
    std::string seed;     // As 12 palavras mnemônicas
    std::string privKey;  // Chave privada derivada da seed

    // Construtor padrão
    Wallet(); 

    /**
     * GERAÇÃO DE CHAVES
     */

    // Gera uma nova carteira do zero (padrão BIP-39)
    void generateKey();

    // Recupera uma carteira através de uma seed existente
    void fromSeed(const std::string& existingSeed); 

    // 🔥 ADICIONADO: Método necessário para o comando 'send' do main.cpp
    // Faz a mesma função do fromSeed, mas com o nome esperado pelo compilador
    void generateKeyFromSeed(const std::string& seed_phrase);

    /**
     * PERSISTÊNCIA
     */

    // Carrega os dados da carteira de um arquivo
    bool loadFromFile(const std::string& path);

    // Salva os dados da carteira em um arquivo local
    bool saveToFile(const std::string& path);

    /**
     * MÉTODOS DE ACESSO (Compatibilidade com main.cpp)
     */

    // Retorna o endereço (MZ...)
    std::string getAddress() const { return address; }

    // Retorna a Seed Phrase
    std::string getSeed() const { return seed; }

    // Retorna a Chave Privada (Necessário para assinar transações no main.cpp)
    std::string getPrivateKey() const { return privKey; }

    // Retorna o endereço público (Antigo padrão)
    std::string getPublicKey() const { return address; }

    /**
     * MÉTODOS FINANCEIROS
     */

    // Retorna o saldo atual desta carteira consultando a Blockchain
    double getBalance(const Blockchain& bc) const {
        return bc.getBalance(this->address);
    }

    /**
     * ASSINATURA DIGITAL
     */

    // Assina uma mensagem (ou hash de transação) usando a chave privada
    std::string sign(const std::string& message);

    // Verifica se a carteira possui chaves válidas carregadas
    bool isValid() const;

    // ── HD Wallet (BIP-32) ────────────────────────────────────────────────────
    // Deriva um endereço filho no caminho m/44'/1611'/0'/0/<index>
    HDWallet::HDKey deriveChild(uint32_t index) const;

    // Gera N endereços filhos a partir desta seed (gap limit)
    std::vector<HDWallet::HDKey> deriveAddresses(int count = 20) const;
};

#endif // WALLET_H completo