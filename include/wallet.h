#ifndef WALLET_H
#define WALLET_H

#include <string>
#include <vector>

class Wallet {
public:
    std::string address;  // Endereço público (ex: MZ...)
    std::string seed;     // As 12 palavras
    std::string privKey;  // Chave privada derivada da seed (para assinar)

    // Construtor padrão
    Wallet(); 

    // Gera uma nova carteira do zero
    void create(); 

    // Recupera uma carteira através de uma seed existente
    void fromSeed(const std::string& existingSeed); 

    /**
     * ASSINATURA DIGITAL (Robustez)
     * Em vez de enviar a seed para a rede, a carteira gera um 
     * código único (assinatura) para a transação.
     */
    std::string sign(const std::string& message);

    // Função auxiliar para verificar se a carteira está carregada
    bool isValid() const;
};

#endif