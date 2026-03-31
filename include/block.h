#ifndef BLOCK_H
#define BLOCK_H

#include <string>
#include <vector>

class Block {
private:
    int index;
    std::string timestamp;
    std::vector<std::string> transactions;
    std::string previousHash;
    std::string hash;
    int nonce;

public:
    Block(int idx, const std::vector<std::string>& txs, const std::string& prevHash);

    std::string calculateHash() const;
    void mineBlock(int difficulty);

    // Getters
    int getIndex() const;
    const std::string& getTimestamp() const;
    const std::vector<std::string>& getTransactions() const;
    const std::string& getPreviousHash() const;
    const std::string& getHash() const;
    int getNonce() const;

    // Setter controlado (necessário)
    void setPreviousHash(const std::string& prevHash);
};

#endif
