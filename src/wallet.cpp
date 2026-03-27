#include "../include/wallet.h"
#include "../include/crypto.h"

Wallet createWallet() {
    Wallet w;
    w.privateKey = generatePrivateKey();
    w.address = "MC-" + sha256(w.privateKey).substr(0, 20);
    return w;
}
