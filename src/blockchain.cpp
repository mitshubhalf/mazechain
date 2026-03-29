#include "blockchain.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>

std::string Blockchain::sha256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)input.c_str(), input.size(), hash);

    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

Blockchain::Blockchain() {
    load();

    if(chain.empty()) {
        Block genesis;
        genesis.index = 0;
        genesis.data = "Genesis";
        genesis.prevHash = "0";
        genesis.hash = sha256("genesis");

        chain.push_back(genesis);
        save();
    }
}

void Blockchain::mineBlock(const std::string& miner) {
    Block b;
    b.index = chain.size();
    b.prevHash = chain.back().hash;

    std::stringstream ss;
    ss << miner << " +250";
    b.data = ss.str();

    b.hash = sha256(b.data + b.prevHash);

    chain.push_back(b);
    save();

    std::cout << "Bloco #" << b.index << " minerado!\n";
}

int Blockchain::getBalance(const std::string& address) {
    int balance = 0;

    for(auto &b : chain) {
        if(b.data.find(address) != std::string::npos) {
            balance += 250;
        }
    }

    return balance;
}

void Blockchain::save() {
    std::ofstream file("chain.txt");

    for(auto &b : chain) {
        file << b.index << "|" << b.data << "|" << b.prevHash << "|" << b.hash << "\n";
    }
}

void Blockchain::load() {
    std::ifstream file("chain.txt");

    if(!file) return;

    std::string line;

    while(std::getline(file, line)) {
        Block b;

        size_t p1 = line.find("|");
        size_t p2 = line.find("|", p1+1);
        size_t p3 = line.find("|", p2+1);

        b.index = std::stoi(line.substr(0, p1));
        b.data = line.substr(p1+1, p2-p1-1);
        b.prevHash = line.substr(p2+1, p3-p2-1);
        b.hash = line.substr(p3+1);

        chain.push_back(b);
    }
}

bool Blockchain::isValid() {
    for(size_t i = 1; i < chain.size(); i++) {
        if(chain[i].prevHash != chain[i-1].hash) return false;
        if(chain[i].hash != sha256(chain[i].data + chain[i].prevHash)) return false;
    }
    return true;
}
