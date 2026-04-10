#ifndef STORAGE_H
#define STORAGE_H

#include "blockchain.h"
#include <vector>
#include <string>

class Storage {
public:
    static void saveChain(const Blockchain& bc, const std::string& filename);
    static void loadChain(Blockchain& bc, const std::string& filename);
    
    // Novas funções para Mempool
    static void saveMempool(const Transaction& tx, const std::string& filename);
    static std::vector<Transaction> loadMempool(const std::string& filename);
    static void clearMempool(const std::string& filename);
};

#endif
