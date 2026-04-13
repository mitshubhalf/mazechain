#ifndef CRYPTO_H
#define CRYPTO_H

#include <string>
#include <vector>
#include "transaction.h"

namespace Crypto {
    std::string sha256_util(std::string str);
    std::string calculateMerkleRoot(const std::vector<Transaction>& txs);
}

#endif
