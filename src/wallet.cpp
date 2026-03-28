#include "../include/wallet.h"
#include "../include/crypto.h"
#include <cstdlib>

Wallet createWallet() {
    std::string priv = std::to_string(rand()) + std::to_string(rand());
    std::string address = "MC-" + sha256simple(priv).substr(0, 20);

    return {address, priv};
}
