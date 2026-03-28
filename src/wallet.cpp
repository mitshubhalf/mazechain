#include "../include/wallet.h"
#include "../include/crypto.h"
#include <sstream>
#include <iomanip>

Wallet createWallet() {
    Wallet w;

    // gera chave privada
    w.privateKey = generatePrivateKey();

    // gera chave pública
    std::string pubKey = getPublicKey(w.privateKey);

    // cria endereço simples (hash da pubkey)
    std::stringstream ss;
    ss << "MC-";

    for (int i = 0; i < 10 && i < pubKey.size(); i++) {
        ss << std::hex << (int)pubKey[i];
    }

    w.address = ss.str();

    return w;
}
