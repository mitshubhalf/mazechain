#ifndef CRYPTO_H
#define CRYPTO_H

#include <string>

namespace Crypto {

    std::string sha256(const std::string &input);

    // 🔁 Usado em blockchains como Bitcoin
    std::string doubleSHA256(const std::string &input);

}

#endif
