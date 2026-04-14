#ifndef STORAGE_H
#define STORAGE_H

#include <string>
#include <vector>
#include "transaction.h"

// Forward declaration da classe Blockchain
// Isso evita que o Storage tente incluir o Blockchain que tenta incluir o Storage (loop)
class Blockchain; 

class Storage {
public:
    // Persistência da Corrente de Blocos
    static void saveChain(const Blockchain& bc, const std::string& filename);
    
    // Retorna true se o arquivo existir e for carregado com sucesso
    static bool loadChain(Blockchain& bc, const std::string& filename);
    
    // Gestão da Mempool (Transações aguardando mineração)
    static void saveMempool(const Transaction& tx, const std::string& filename);
    static std::vector<Transaction> loadMempool(const std::string& filename);
    static void clearMempool(const std::string& filename);
};

#endif
