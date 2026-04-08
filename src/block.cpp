#include "../include/block.h"
#include <sstream>
#include <openssl/sha.h>

Block::Block(int idx, std::vector<Transaction> txs, std::string prevHash) {
    index = idx;
    transactions = txs;
    previousHash = prevHash;
    nonce = 0;
    hash = calculateHash();
}

std::string Block::calculateHash() const {

    std::stringstream ss;

    ss << index << previousHash << nonce;

    for (const auto& tx : transactions) {
        ss << tx.id;

        for (const auto& in : tx.inputs) {
            ss << in.txId << in.outputIndex << in.address;
        }

        for (const auto& out : tx.outputs) {
            ss << out.address << out.amount;
        }
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)ss.str().c_str(), ss.str().size(), hash);

    std::stringstream hex;

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        hex << std::hex << (int)hash[i];
    }

    return hex.str();
}

void Block::mineBlock(int difficulty) {
    std::string target(difficulty, '0');

    while (hash.substr(0, difficulty) != target) {
        nonce++;
        hash = calculateHash();
    }
}
