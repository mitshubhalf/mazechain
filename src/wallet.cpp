#include "../include/wallet.h"
#include "../include/crypto.h" // Alterado para usar o seu header de criptografia
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>

void Wallet::create() {
    // Lista de palavras (BIP-39 style)
    std::vector<std::string> wordlist = {
        "abandon", "ability", "able", "about", "above", "absent", "absorb", "abstract", 
        "absurd", "abuse", "access", "accident", "account", "accuse", "achieve", "acid", 
        "acoustic", "acquire", "across", "act", "action", "actor", "actress", "actual"
    }; 

    // Gerador de números aleatórios
    std::random_device rd;
    std::mt19937 g(rd());
    
    // Embaralha a lista de forma segura
    std::shuffle(wordlist.begin(), wordlist.end(), g);

    // Limpa a seed anterior
    this->seed = "";
    
    // Constrói a frase de 12 palavras
    for(int i = 0; i < 12; ++i) {
        this->seed += wordlist[i];
        if (i < 11) this->seed += " "; 
    }

    // --- GERAÇÃO DO ENDEREÇO SEGURO ---
    // IMPORTANTE: Usei Crypto::sha256 que é a função que definimos no crypto.cpp
    
    // 1. Hash da Seed
    std::string seedHash = Crypto::sha256(this->seed);
    
    // 2. Hash do Hash (Double SHA256) - Padrão Bitcoin de segurança
    std::string secureHash = Crypto::sha256(seedHash);
    
    // Define o endereço final: Prefixo MZ + 20 caracteres do hash
    this->address = "MZ" + secureHash.substr(0, 20); 

    // Log de auditoria no servidor
    std::cout << "[NODE] Nova Carteira: " << this->address << std::endl;
}
