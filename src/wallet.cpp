#include "../include/wallet.h"
#include "../include/blockchain.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>

void Wallet::create() {
    std::vector<std::string> wordlist = {
        "abandon", "ability", "able", "about", "above", "absent", 
        "absorb", "abstract", "absurd", "abuse", "access", "accident",
        "account", "accuse", "achieve", "acid", "acoustic", "acquire"
        // ... adicione mais palavras se desejar
    }; 

    std::random_device rd;
    std::mt19937 g(rd());
    
    // Garantir que temos palavras suficientes para o shuffle
    std::shuffle(wordlist.begin(), wordlist.end(), g);

    // Limpa a seed atual e gera a nova
    this->seed = "";
    for(int i = 0; i < 12; ++i) {
        this->seed += wordlist[i] + (i == 11 ? "" : " ");
    }

    // --- SISTEMA DE ENDEREÇO ---
    std::string seedHash = sha256_util(this->seed);
    std::string secureHash = sha256_util(seedHash);
    
    this->address = "MZ" + secureHash.substr(0, 20); 

    // O log no console continua para debug do Render
    std::cout << "\n[DEBUG] Nova Carteira: " << this->address << std::endl;
}
