#ifndef WALLET_H
#define WALLET_H

#include <string>
#include <vector>

class Wallet {
public:
    std::string address;
    
    // Esta função gera a seed de 2048 palavras e o endereço
    void create(); 
};

#endif
