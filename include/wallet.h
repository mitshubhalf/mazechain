#pragma once
#include <string>

struct Wallet {
    std::string address;
    std::string privateKey;
};

Wallet createWallet();
