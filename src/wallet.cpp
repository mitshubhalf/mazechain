#include "../include/wallet.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <cstdlib>

std::string sha256simple(std::string input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)input.c_str(), input.size(), hash);

    std::stringstream ss;
    for(int i=0;i<SHA256_DIGEST_LENGTH;i++)
        ss<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)hash[i];

    return ss.str();
}

Wallet createWallet() {

    std::string priv = std::to_string(rand()) + std::to_string(rand());

    std::string address = "MC-" + sha256simple(priv).substr(0,20);

    return {address, priv};
}
