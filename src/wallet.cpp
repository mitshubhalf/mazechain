#include "../include/wallet.h"
#include "../include/crypto.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <ctime>

Wallet::Wallet() {
    std::srand(std::time(nullptr));
    generateMnemonic();
    generateFromMnemonic();
}

static std::vector<std::string> wordList = {
    "apple","banana","cat","dog","earth","fire","gold","house",
    "ice","jungle","king","lemon","moon","night","ocean","peace",
    "queen","river","sun","tree","unity","victory","wind","xenon",
    "yellow","zebra"
};

void Wallet::generateMnemonic() {
    mnemonic.clear();

    for (int i = 0; i < 12; i++) {
        mnemonic += wordList[rand() % wordList.size()];
        if (i < 11) mnemonic += " ";
    }
}

void Wallet::generateFromMnemonic() {
    privateKey = sha256(mnemonic);
    publicKey = sha256(privateKey);
    address = sha256(publicKey).substr(0, 40);
}

void Wallet::saveToFile(const std::string& filename) {
    std::ofstream f(filename);

    if (!f.is_open()) return;

    f << mnemonic << "\n";
    f << privateKey << "\n";
    f << publicKey << "\n";
    f << address << "\n";
}

void Wallet::loadFromFile(const std::string& filename) {
    std::ifstream f(filename);

    if (!f.is_open()) return;

    std::getline(f, mnemonic);
    std::getline(f, privateKey);
    std::getline(f, publicKey);
    std::getline(f, address);
}
