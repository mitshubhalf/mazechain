#ifndef WALLET_H
#define WALLET_H

#include <string>
#include <vector>

class Wallet {
public:
    std::string address;
    std::string seed; // <--- ADICIONADO: Permite que a API acesse a seed

    void create(); 
};

#endif
