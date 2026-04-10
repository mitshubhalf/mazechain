#include "../include/wallet.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <fstream>

// Simulação da lista BIP-39 (Na vida real, você carrega um arquivo com 2048 palavras)
std::vector<std::string> loadWordlist() {
    std::vector<std::string> words = {"abandon", "ability", "able", "about", "above", "absent", "absorb", "abstract", "absurd", "abuse", "access", "accident"}; 
    // ... imagine 2048 palavras aqui ...
    return words;
}

void Wallet::create() {
    std::vector<std::string> wordlist = loadWordlist();
    
    // 1. Configura o gerador de números aleatórios com entropia real
    std::random_device rd;
    std::mt19937 g(rd());

    // 2. EMBARALHAMENTO (Shuffle): A melhor forma de não repetir
    // Em vez de sortear um por um, embaralhamos a lista inteira e pegamos as primeiras 12
    std::shuffle(wordlist.begin(), wordlist.end(), g);

    std::string seedFrase = "";
    for(int i = 0; i < 12; ++i) {
        seedFrase += wordlist[i] + (i == 11 ? "" : " ");
    }

    // 3. EXIBIÇÃO
    std::cout << "📝 Nova Seed Gerada (12 palavras únicas):" << std::endl;
    std::cout << "------------------------------------------" << std::endl;
    std::cout << seedFrase << std::endl;
    std::cout << "------------------------------------------" << std::endl;
    
    // Aqui você chamaria a função para derivar a Private Key da Seed
    this->address = generateAddressFromSeed(seedFrase);
    std::cout << "💳 Endereço: " << this->address << std::endl;
    
    // Salva a carteira em arquivo para persistência
    saveToFile(seedFrase);
}
