#ifndef WALLET_H
#define WALLET_H

#include <string>

class Wallet {
public:
    std::string address;
    std::string seed;

    void create(); 
};

#endif
