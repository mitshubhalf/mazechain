#include "../include/crypto.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

namespace Crypto {

std::string sha256(const std::string &input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    
    // Converte a string de entrada para o formato que a OpenSSL espera
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);

    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        // Formata cada byte como um hexadecimal de 2 dígitos (ex: 0a, 1f)
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

}
