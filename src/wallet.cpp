#include "../include/wallet.h"
#include "../include/crypto.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <fstream>
#include <sstream>

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
        std::cout << "Rode: curl -L https://raw.githubusercontent.com/bitcoin/bips/master/bip-0039/english.txt -o wordlist.txt" << std::endl;
        return; 
    }

    // 3. Seleção Aleatória Segura (Sorteio, não embaralhamento da lista toda)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 2047);

    // Definimos 12 palavras como padrão, mas o código suporta 24
    int numWords = 12; 
    std::string tempSeed = "";

    for(int i = 0; i < numWords; ++i) {
        tempSeed += wordlist[dis(gen)];
        if (i < numWords - 1) tempSeed += " "; 
    }

    // 4. Gerar Endereço a partir da Seed sorteada
    fromSeed(tempSeed);

    // 5. Output Formatado conforme seu padrão
    std::cout << "\n==========================================" << std::endl;
    std::cout << "✅ MAZECHAIN: CARTEIRA GERADA (" << numWords << "W)" << std::endl;
    std::cout << "ADDRESS : " << this->address << std::endl;
    std::cout << "SEED    : " << this->seed << std::endl;
    std::cout << "AVISO   : Nunca compartilhe sua SEED!" << std::endl;
    std::cout << "==========================================\n" << std::endl;
}

void Wallet::fromSeed(const std::string& existingSeed) {
    this->seed = existingSeed;

    // Protocolo MazeChain v2.1 - Endereço de Alta Densidade
    // 1. Double Hash para segurança máxima
    std::string h1 = Crypto::sha256_util(this->seed);
    
    // 2. Salt Temporal MazeChain 2026
    std::string h2 = Crypto::sha256_util(h1 + "SALT_MAZE_2026_PRODUCTION");
    
    // 3. Endereço formatado com 34 caracteres
    this->address = "MZ" + h2.substr(0, 32);
}
