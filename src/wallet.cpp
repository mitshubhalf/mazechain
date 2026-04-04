#include "../include/crypto.h"
#include <string>

class Wallet {
public:
    std::string privateKey;

    void generateFromMnemonic(const std::string& mnemonic) {
        // 🔥 CORREÇÃO: usar namespace Crypto
        privateKey = Crypto::sha256(mnemonic);
    }
};
