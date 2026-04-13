#include "../include/wallet.h"
#include "../include/crypto.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>

void Wallet::create() {
    // Sua lista de palavras original
    std::vector<std::string> wordlist = {
        "abandon", "ability", "able", "about", "above", "absent", "absorb", "abstract", 
        "absurd", "abuse", "access", "accident", "account", "accuse", "achieve", "acid", 
        "acoustic", "acquire", "across", "act", "action", "actor", "actress", "actual"
    }; 

    // Embaralhamento seguro para gerar a seed
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(wordlist.begin(), wordlist.end(), g);

    std::string tempSeed = "";
    for(int i = 0; i < 12; ++i) {
        tempSeed += wordlist[i];
        if (i < 11) tempSeed += " "; 
    }

    // Agora utilizamos o método fromSeed para definir o endereço MZ
    // Isso garante que a lógica de hash seja centralizada em um só lugar
    fromSeed(tempSeed);

    std::cout << "\n✅ NODE: NOVA CARTEIRA CRIPTOGRAFADA" << std::endl;
    std::cout << "ADDRESS: " << this->address << std::endl;
    std::cout << "SEED   : " << this->seed << std::endl;
    std::cout << "------------------------------------------" << std::endl;
}

void Wallet::fromSeed(const std::string& existingSeed) {
    // Armazena a semente fornecida
    this->seed = existingSeed;

    // Protocolo de Geração de Endereço MazeChain v2.0
    // 1. Gera o hash da semente
    std::string seedHash = Crypto::sha256_util(this->seed);
    
    // 2. Aplica Double SHA256 para maior segurança contra ataques de colisão
    std::string secureHash = Crypto::sha256_util(seedHash);
    
    // 3. Define o endereço público com o prefixo da rede (MZ)
    // Pegamos os primeiros 20 caracteres do hash final
    this->address = "MZ" + secureHash.substr(0, 20); 
}
