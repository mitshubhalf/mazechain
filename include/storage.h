#ifndef STORAGE_H
#define STORAGE_H

#include <string>
#include <vector>
#include "transaction.h" // Necessário para saveMempool e loadMempool

// Forward Declaration (Aviso ao compilador que a classe existe sem carregar o arquivo todo agora)
class Blockchain; 

class Storage {
public:
    // Métodos para a Corrente
    static void saveChain(const Blockchain& bc, const std::string& filename);
    static void loadChain(Blockchain& bc, const std::string& filename);
    
    // Métodos para a Mempool
    static void saveMempool(const Transaction& tx, const std::string& filename);
    static std::vector<Transaction> loadMempool(const std::string& filename);
    static void clearMempool(const std::string& filename);
};

#endif
