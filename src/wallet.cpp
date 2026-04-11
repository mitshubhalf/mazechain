#include "../include/wallet.h"
#include "../include/blockchain.h" // Para usar o sha256_util
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/pem.h>

void Wallet::create() {
    std::vector<std::string> wordlist = {
        "abandon", "ability", "able", "about", "above", "absent", 
        "absorb", "abstract", "absurd", "abuse", "access", "accident",
        "account", "accuse", "achieve", "acid", "acoustic", "acquire"
    }; 

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(wordlist.begin(), wordlist.end(), g);

    std::string seed = "";
    for(int i = 0; i < 12; ++i) {
        seed += wordlist[i] + (i == 11 ? "" : " ");
    }

    // --- NOVO SISTEMA DE ENDEREÇO SEGURO ---
    // 1. Geramos um Hash da Seed para servir de "Entropia"
    std::string seedHash = sha256_util(seed);
    
    // 2. Criamos um endereço muito mais longo e único
    // Pegamos o Hash da seed, aplicamos sha256 de novo e pegamos os 20 primeiros caracteres
    std::string secureHash = sha256_util(seedHash);
    this->address = "MZ" + secureHash.substr(0, 20); 

    std::cout << "\n==========================================" << std::endl;
    std::cout << "📝 SEED GERADA:" << std::endl;
    std::cout << seed << std::endl;
    std::cout << "------------------------------------------" << std::endl;
    std::cout << "💳 Endereço Seguro: " << this->address << std::endl;
    std::cout << "🔑 Chave Privada (Hash): " << seedHash << std::endl;
    std::cout << "==========================================\n" << std::endl;
}
