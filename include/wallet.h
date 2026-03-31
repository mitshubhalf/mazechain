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

// 🔤 Lista simples de palavras (mock de BIP39)
static std::vector<std::string> wordList = {
    "apple", "banana", "cat", "dog", "earth", "fire", "gold", "house",
    "ice", "jungle", "king", "lemon", "moon", "night", "ocean", "peace",
    "queen", "river", "sun", "tree", "unity", "victory", "wind", "xenon",
    "yellow", "zebra"
};

void Wallet::generateMnemonic() {
    mnemonic.clear();

    for (int i = 0; i < 12; i++) {
        int index = rand() % wordList.size();
        mnemonic += wordList[index];

        if (i < 11) mnemonic += " ";
    }
}

void Wallet::generateFromMnemonic() {
    // 🔐 Derivação simples (didática)
    privateKey = sha256(mnemonic);
    publicKey = sha256(privateKey);
    address = sha256(publicKey).substr(0, 40); // tipo Ethereum
}

void Wallet::saveToFile(const std::string& filename) {
    std::ofstream file(filename);

    if (!file.is_open()) return;

    file << mnemonic << "\n";
    file << privateKey << "\n";
    file << publicKey << "\n";
    file << address << "\n";
}

void Wallet::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) return;

    std::getline(file, mnemonic);
    std::getline(file, privateKey);
    std::getline(file, publicKey);
    std::getline(file, address);
}

// Usa sua implementação de SHA256
std::string Wallet::sha256(const std::string& input) {
    return ::sha256(input);
}
