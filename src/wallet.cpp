#include "../include/wallet.h"
#include "../include/crypto.h"
#include <fstream>
#include <iostream>

// ==========================
// CONSTRUTOR
// ==========================
Wallet::Wallet() {}

// ==========================
// GERAR MNEMONIC SIMPLES
// ==========================
void Wallet::generateMnemonic() {
    mnemonic = "seed-mazechain-" + std::to_string(rand());
}

// ==========================
// GERAR CHAVES
// ==========================
void Wallet::generateFromMnemonic() {

    privateKey = Crypto::sha256(mnemonic);
    publicKey  = Crypto::sha256(privateKey);
    address    = Crypto::sha256(publicKey).substr(0, 40);
}

// ==========================
// SALVAR WALLET
// ==========================
void Wallet::saveToFile(const std::string& filename) {

    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cout << "Erro ao salvar wallet\n";
        return;
    }

    file << mnemonic << "\n";
    file << privateKey << "\n";
    file << publicKey << "\n";
    file << address << "\n";
}

// ==========================
// CARREGAR WALLET
// ==========================
void Wallet::loadFromFile(const std::string& filename) {

    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cout << "Erro ao carregar wallet\n";
        return;
    }

    std::getline(file, mnemonic);
    std::getline(file, privateKey);
    std::getline(file, publicKey);
    std::getline(file, address);
}
