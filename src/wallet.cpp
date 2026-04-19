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
}

void Wallet::create() {
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
}

// ✅ MELHORIA: Implementação da função Sign necessária para transações
std::string Wallet::sign(const std::string& message) {
    // Em uma blockchain real, usaríamos ECDSA (Chave Privada).
    // Para a MazeChain v2.1, usamos um Hash Assinado (HMAC-style)
    // que prova que você possui a SEED original.
    std::string signature = Crypto::sha256_util(this->seed + message + "MAZE_SIG_V2");
    return signature;
}
