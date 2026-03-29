#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <string>

struct Block {
    int index;
    std::string data;
    std::string prevHash;
    std::string hash;
};

class Blockchain {
public:
    std::vector<Block> chain;

    Blockchain();

    void mineBlock(const std::string& miner);
    int getBalance(const std::string& address);

    void save();
    void load();

    bool isValid();

private:
    std::string sha256(const std::string& input);
};

#endif
