#ifndef BLOCK_H
#define BLOCK_H

#include <string>
#include <vector>

class Block {
public:
    int index;
    std::string timestamp;
    std::vector<std::string> transactions;
    std::string previousHash;
    std::string hash;
    int nonce;

    Block(int idx, std::vector<std::string> txs, std::string prevHash);

    std::string calculateHash();
    void mineBlock(int difficulty);
};

#endif
