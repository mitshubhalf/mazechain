#include "../include/wallet.h"
#include "../include/crypto.h"
#include <sstream>

Wallet createWallet() {
    Wallet w;

    std::string privKey = generatePrivateKey();
    std::string pubKey = getPublicKey(privKey);

    std::string address = sha256(pubKey).substr(0, 20);

    w.address = "MC-" + address;
    w.privateKey = privKey;

    return w;
}
