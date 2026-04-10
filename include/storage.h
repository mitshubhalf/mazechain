#ifndef STORAGE_H
#define STORAGE_H

#include <string>
#include <vector>
#include "blockchain.h"

class Storage {
public:
    static void saveChain(const Blockchain& bc, const std::string& filename);
    static void loadChain(Blockchain& bc, const std::string& filename);
    static void saveMempool(const Transaction& tx, const std::string& filename);
    static std::vector<Transaction> loadMempool(const std::string& filename);
    static void clearMempool(const std::string& filename);
};

#endif
