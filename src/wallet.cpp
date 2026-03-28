#include "../include/wallet.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <random>

std::string randomString() {
    std::string chars = "abcdef0123456789";
    std::string result;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);

    for (int i = 0; i < 32; i++) {
        result += chars[dis(gen)];
    }

    return result;
}

std::string sha256(std::string input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)input.c_str(), input.size(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }

    return ss.str();
}

Wallet createWallet() {
    std::string seed = randomString();
    std::string address = "MC-" + sha256(seed).substr(0, 20);

    Wallet w;
    w.address = address;

    return w;
}
