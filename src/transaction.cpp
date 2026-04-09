#include "../include/transaction.h"
#include <sstream>
#include <openssl/sha.h>

Transaction::Transaction(std::vector<TxIn> in, std::vector<TxOut> out)
    : vin(in), vout(out) {
    id = calculateHash();
}

std::string Transaction::calculateHash() const {
    std::stringstream ss;

    for (auto &i : vin)
        ss << i.txid << i.index;

    for (auto &o : vout)
        ss << o.address << o.amount;

    std::string data = ss.str();

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)data.c_str(), data.size(), hash);

    std::stringstream result;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        result << std::hex << (int)hash[i];

    return result.str();
}
