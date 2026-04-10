#ifndef WALLET_H
#define WALLET_H

#include <string>
#include <vector>

class Wallet {
public:
    std::string address;
    
    // Esta linha é a que estava faltando:
    void create(); 
    
    // Funções auxiliares que você pode precisar
    bool load();
    void save(const std::string& seed);
};

#endif
