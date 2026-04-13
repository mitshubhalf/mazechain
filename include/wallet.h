#ifndef WALLET_H
#define WALLET_H

#include <string>

class Wallet {
public:
    std::string address;
    std::string seed;

    void create(); 
    // Novo método para evitar o erro de compilação
    void fromSeed(const std::string& existingSeed); 
};

#endif
