#ifndef STORAGE_H
#define STORAGE_H

#include <string>
#include <vector>
#include "transaction.h"

class Blockchain; 

class Storage {
public:
    static void saveChain(const Blockchain& bc, const std::string& filename);
    // Alterado para bool para o main.cpp funcionar
    static bool loadChain(Blockchain& bc, const std::string& filename);
    
    static void saveMempool(const Transaction& tx, const std::string& filename);
    static std::vector<Transaction> loadMempool(const std::string& filename);
    static void clearMempool(const std::string& filename);
};

#endif
