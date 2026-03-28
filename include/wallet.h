#ifndef WALLET_H
#define WALLET_H

#include <string>

struct Wallet {
    std::string address;
    std::string privateKey;
};

Wallet createWallet();

#endif
