#include "../include/wallet.h"
#include <sstream>
#include <iomanip>
#include <random>

Wallet createWallet() {
    std::stringstream ss;
    ss << "MC-";

    std::random_device rd;
    for (int i = 0; i < 20; i++) {
        ss << std::hex << (rd() % 16);
    }

    return {ss.str()};
}
