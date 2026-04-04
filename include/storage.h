#ifndef STORAGE_H
#define STORAGE_H

#include "blockchain.h"

namespace Storage {

void saveChain(const Blockchain &bc, const std::string& filename = "chain.txt");
void loadChain(Blockchain &bc, const std::string& filename = "chain.txt");

}

#endif
