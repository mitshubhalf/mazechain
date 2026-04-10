#include "../include/wallet.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>

void Wallet::create() {
    // Exemplo de lista. Para 2048, você carregaria de um arquivo .txt
    std::vector<std::string> wordlist = {"abandon", "ability", "able", "about", "above", "absent", "absorb", "abstract", "absurd", "abuse", "access", "accident", "account", "accuse", "achieve", "acid", "acoustic", "acquire", "across", "act", "action", "actor", "actress", "actual"}; 

    std::random_device rd;
    std::mt19937 g(rd());

    // EMBARALHAMENTO: Garante que não há repetição
    std::shuffle(wordlist.begin(), wordlist.end(), g);

    std::string seed = "";
    for(int i = 0; i < 12; ++i) {
        seed += wordlist[i] + (i == 11 ? "" : " ");
    }

    std::cout << "📝 Seed (12 palavras únicas): " << seed << std::endl;
    this->address = "MZ" + seed.substr(0, 8); // Simplificação do endereço
    std::cout << "💳 Addr: " << this->address << std::endl;
}
