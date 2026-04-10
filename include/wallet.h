#ifndef WALLET_H
#define WALLET_H

#include <string>
#include <vector>

class Wallet {
public:
    // static permite chamar sem criar um objeto "Wallet w;"
    static std::string generateMnemonic(); 
    static std::string deriveAddress(std::string mnemonic);
};

#endif
