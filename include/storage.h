#ifndef STORAGE_H
#define STORAGE_H

#include "blockchain.h"

void saveChain(const Blockchain &bc);
void loadChain(Blockchain &bc);

#endif
