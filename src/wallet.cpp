#include "../include/wallet.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>

void Wallet::create() {
    // Lista para sorteio (Wordlist de 2048 palavras)
    // Aqui você pode colocar a lista BIP-39 completa depois
    std::vector<std::string> wordlist = {
        "abandon", "ability", "able", "about", "above", "absent", 
        "absorb", "abstract", "absurd", "abuse", "access", "accident"
        // ... adicione mais palavras se quiser testar ...
    }; 

    std::random_device rd;
    std::mt19937 g(rd());

    // Embaralha para garantir que não repete
    std::shuffle(wordlist.begin(), wordlist.end(), g);

    std::string seed = "";
    for(int i = 0; i < 12; ++i) {
        seed += wordlist[i] + (i == 11 ? "" : " ");
    }

    std::cout << "\n==========================================" << std::endl;
    std::cout << "📝 SEED GERADA (Guarde em local seguro):" << std::endl;
    std::cout << seed << std::endl;
    std::cout << "==========================================\n" << std::endl;

    // Gera um endereço baseado nas primeiras letras da seed (exemplo simples)
    this->address = "MZ" + wordlist[0].substr(0, 3) + wordlist[1].substr(0, 3);
    std::cout << "💳 Endereço da Carteira: " << this->address << std::endl;
}
