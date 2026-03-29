#ifndef WALLET_H
#define WALLET_H

#include <string>

class Wallet {
public:
    std::string mnemonic;
    std::string privateKey;
    std::string publicKey;
    std::string address;

    Wallet();

    void generateMnemonic();
    void generateFromMnemonic();

    void saveToFile(const std::string& filename);
    void loadFromFile(const std::string& filename);

private:
    std::string sha256(const std::string& input);
};

#endif
