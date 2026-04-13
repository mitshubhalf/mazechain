#ifndef WALLET_H
#define WALLET_H

#include <string>

class Wallet {
public:
    std::string address;
    std::string seed;

    // Gera uma seed aleatória e um endereço
    void create(); 
    
    // Recupera o endereço a partir de uma seed existente
    void fromSeed(const std::string& existingSeed); 
};

#endif
