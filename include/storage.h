#ifndef STORAGE_H
#define STORAGE_H

#include <vector>
#include <string>

// Forward Declaration: Avisa ao compilador que essa classe existe
// sem precisar dar o include do blockchain.h aqui.
class Blockchain;
struct Transaction;

class Storage {
public:
    static void saveChain(const Blockchain& bc, const std::string& filename);
    static void loadChain(Blockchain& bc, const std::string& filename);
    
    static void saveMempool(const Transaction& tx, const std::string& filename);
    static std::vector<Transaction> loadMempool(const std::string& filename);
    static void clearMempool(const std::string& filename);
};

#endif
