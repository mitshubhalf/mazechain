#ifndef WALLET_H
#define WALLET_H

#include <string>

class Wallet {
public:
    std::string address;
    std::string seed; // Onde as 12 palavras ficam guardadas

    void create(); 
};

#endif
