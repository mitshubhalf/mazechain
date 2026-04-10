#include "../include/wallet.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>

void Wallet::create() {
    // Lista BIP-39 simplificada (você pode expandir para 2048 palavras)
    std::vector<std::string> wordlist = {
        "abandon", "ability", "able", "about", "above", "absent", 
        "absorb", "abstract", "absurd", "abuse", "access", "accident",
        "account", "accuse", "achieve", "acid", "acoustic", "acquire"
    }; 

    std::random_device rd;
    std::mt19937 g(rd());

    // EMBARALHAMENTO: Garante que as palavras sorteadas sejam únicas
    std::shuffle(wordlist.begin(), wordlist.end(), g);

    std::string seed = "";
    for(int i = 0; i < 12; ++i) {
        seed += wordlist[i] + (i == 11 ? "" : " ");
    }

    // Gera um endereço MZ baseado na seed (Exemplo simplificado)
    this->address = "MZ" + wordlist[0].substr(0, 3) + wordlist[1].substr(0, 3);

    std::cout << "\n==========================================" << std::endl;
    std::cout << "📝 SEED GERADA (12 palavras únicas):" << std::endl;
    std::cout << seed << std::endl;
    std::cout << "------------------------------------------" << std::endl;
    std::cout << "💳 Endereço da Carteira: " << this->address << std::endl;
    std::cout << "==========================================\n" << std::endl;
}
