#include "../include/wallet.h"
#include "../include/blockchain.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>

void Wallet::create() {
    // Lista expandida para evitar erros de shuffle
    std::vector<std::string> wordlist = {
        "abandon", "ability", "able", "about", "above", "absent", "absorb", "abstract", 
        "absurd", "abuse", "access", "accident", "account", "accuse", "achieve", "acid", 
        "acoustic", "acquire", "across", "act", "action", "actor", "actress", "actual"
    }; 

    // Gerador de números aleatórios de alta qualidade
    std::random_device rd;
    std::mt19937 g(rd());
    
    // Embaralha a lista
    std::shuffle(wordlist.begin(), wordlist.end(), g);

    // Garante que a variável da classe seja limpa antes de usar
    this->seed = "";
    
    // Constrói a frase de 12 palavras
    for(int i = 0; i < 12; ++i) {
        this->seed += wordlist[i];
        if (i < 11) this->seed += " "; 
    }

    // --- GERAÇÃO DO ENDEREÇO SEGURO ---
    // 1. Hash da Seed
    std::string seedHash = sha256_util(this->seed);
    // 2. Hash do Hash (Double SHA256 para segurança)
    std::string secureHash = sha256_util(seedHash);
    
    // Define o endereço final
    this->address = "MZ" + secureHash.substr(0, 20); 

    // Log de segurança no console do Render (Debug)
    std::cout << "[SERVER] Carteira Gerada: " << this->address << std::endl;
}
