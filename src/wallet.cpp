#include "../include/wallet.h"
#include "../include/crypto.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>

void Wallet::create() {
    std::vector<std::string> wordlist = {
        "abandon", "ability", "able", "about", "above", "absent", "absorb", "abstract", 
        "absurd", "abuse", "access", "accident", "account", "accuse", "achieve", "acid", 
        "acoustic", "acquire", "across", "act", "action", "actor", "actress", "actual"
    }; 

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(wordlist.begin(), wordlist.end(), g);

    std::string generatedSeed = "";
    for(int i = 0; i < 12; ++i) {
        generatedSeed += wordlist[i];
        if (i < 11) generatedSeed += " "; 
    }

    // Chama o novo método para gerar o endereço baseado na seed criada
    fromSeed(generatedSeed);

    std::cout << "\n✅ NOVA CARTEIRA GERADA" << std::endl;
    std::cout << "Endereço: " << this->address << std::endl;
    std::cout << "Seed: " << this->seed << std::endl;
    std::cout << "------------------------------------------" << std::endl;
}

// Implementação do método que faltava
void Wallet::fromSeed(const std::string& existingSeed) {
    this->seed = existingSeed;

    // A geração do endereço DEVE ser a mesma do método create()
    // para garantir que a importação funcione corretamente.
    std::string seedHash = Crypto::sha256_util(this->seed);
    std::string secureHash = Crypto::sha256_util(seedHash); // Double SHA256
    
    // MZ + 20 caracteres do hash
    this->address = "MZ" + secureHash.substr(0, 20); 
}
