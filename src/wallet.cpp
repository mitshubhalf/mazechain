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

    this->seed = "";
    for(int i = 0; i < 12; ++i) {
        this->seed += wordlist[i];
        if (i < 11) this->seed += " "; 
    }

    // Geração do endereço MZ v2.0
    std::string seedHash = Crypto::sha256_util(this->seed);
    std::string secureHash = Crypto::sha256_util(seedHash); // Double SHA256
    
    this->address = "MZ" + secureHash.substr(0, 20); 

    std::cout << "\n✅ NOVA CARTEIRA GERADA" << std::endl;
    std::cout << "Endereço: " << this->address << std::endl;
    std::cout << "Seed: " << this->seed << std::endl;
    std::cout << "------------------------------------------" << std::endl;
}
