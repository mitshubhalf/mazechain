#ifndef WALLET_H
#define WALLET_H

#include <string>

class Wallet {
public:
    std::string address;
    std::string seed;

    // 1. ADICIONE O CONSTRUTOR (Isso resolve o erro "implicitly-declared")
    Wallet(); 

    // 2. MANTENHA AS FUNÇÕES DE GERAÇÃO
    void create(); 
    void fromSeed(const std::string& existingSeed); 

    // 3. ADICIONE A FUNÇÃO DE ASSINATURA (Isso resolve o erro "no declaration matches")
    std::string sign(const std::string& message);
};

#endif
