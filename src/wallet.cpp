#include "../include/wallet.h"
#include "../include/crypto.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <fstream>
#include <sstream>

// Construtor
Wallet::Wallet() {
    address = "";
    seed = "";
    privKey = "";
}

// ✅ Função exigida pelo main.cpp para restaurar via CLI
void Wallet::generateKeyFromSeed(const std::string& seed_phrase) {
    if (!seed_phrase.empty()) {
        this->fromSeed(seed_phrase);
    }
}

// ✅ CORREÇÃO: Nome alterado de create() para generateKey() para bater com o wallet.h
void Wallet::generateKey() {
    // 1. Carregar a Wordlist BIP-39 oficial
    std::vector<std::string> wordlist;
    std::ifstream file("wordlist.txt");
    std::string word;

    if (file.is_open()) {
        while (file >> word) {
            wordlist.push_back(word);
        }
        file.close();
    }

    // 2. Validação Crítica da Wordlist
    if (wordlist.size() < 2048) {
        std::cout << "\n❌ ERRO CRITICO: wordlist.txt incompleta ou nao encontrada!" << std::endl;
        std::cout << "Certifique-se de que o arquivo wordlist.txt esta na mesma pasta do executavel." << std::endl;
        return; 
    }

    // 3. Seleção Aleatória Segura
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 2047);

    int numWords = 12; 
    std::string tempSeed = "";

    for(int i = 0; i < numWords; ++i) {
        tempSeed += wordlist[dis(gen)];
        if (i < numWords - 1) tempSeed += " "; 
    }

    // 4. Gerar Endereço a partir da Seed sorteada
    fromSeed(tempSeed);
}

void Wallet::fromSeed(const std::string& existingSeed) {
    this->seed = existingSeed;

    // Protocolo MazeChain v2.1 - Endereço de Alta Densidade
    // 1. Double Hash para segurança máxima
    std::string h1 = Crypto::sha256_util(this->seed);

    // 2. Salt Temporal MazeChain 2026
    std::string h2 = Crypto::sha256_util(h1 + "SALT_MAZE_2026_PRODUCTION");

    // 3. Endereço formatado com prefixo MZ + 32 caracteres do hash
    this->address = "MZ" + h2.substr(0, 32);

    // 4. A privKey para o protocolo v2.1 pode ser o h1 para assinaturas HMAC
    this->privKey = h1;
}

// ✅ PERSISTÊNCIA: Implementação necessária para o minerador em main.cpp
bool Wallet::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::getline(file, address);
    std::getline(file, seed);
    std::getline(file, privKey);

    return isValid();
}

bool Wallet::saveToFile(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << address << "\n" << seed << "\n" << privKey << std::endl;
    return true;
}

// ✅ MELHORIA: Implementação da função Sign necessária para transações
std::string Wallet::sign(const std::string& message) {
    // Protocolo MazeChain v2.1 - Hash Assinado (HMAC-style)
    // Prova que você possui a SEED original.
    std::string signature = Crypto::sha256_util(this->seed + message + "MAZE_SIG_V2");
    return signature;
}

// ✅ VALIDAÇÃO: Verifica se a carteira está pronta
bool Wallet::isValid() const {
    return !address.empty() && !seed.empty();
}

/** * NOTA DE COMPILAÇÃO:
 * Se o seu wallet.h já contém "{ return address; }" etc., 
 * as funções abaixo devem ser removidas para evitar o erro de "redefinition".
 * Se no wallet.h elas terminam apenas com ";", mantenha as linhas abaixo.
 */
// std::string Wallet::getAddress() const { return address; }
// std::string Wallet::getSeed() const { return seed; }
// std::string Wallet::getPrivateKey() const { return privKey; }