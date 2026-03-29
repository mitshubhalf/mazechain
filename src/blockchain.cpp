#include "blockchain.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>

bool verifySignature(const std::string&, const std::string&, const std::string&);

std::string Blockchain::sha256(const std::string& input){
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)input.c_str(), input.size(), hash);

    std::stringstream ss;
    for(int i=0;i<SHA256_DIGEST_LENGTH;i++)
        ss<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)hash[i];

    return ss.str();
}

Blockchain::Blockchain(){
    load();
    loadMempool(); // 🔥 NOVO

    if(chain.empty()){
        Block g;
        g.index = 0;
        g.data = "GENESIS";
        g.prevHash = "0";
        g.hash = sha256("genesis");

        chain.push_back(g);
        save();
    }
}

bool Blockchain::validateTransaction(const Transaction& tx){

    if(tx.from == "SYSTEM") return true;

    int balance = getBalance(tx.from);

    if(balance < tx.amount){
        std::cout << "Saldo insuficiente\n";
        return false;
    }

    if(!verifySignature(tx.toString(), tx.signature, tx.from)){
        std::cout << "Assinatura inválida\n";
        return false;
    }

    return true;
}

void Blockchain::addTransaction(const Transaction& tx){

    if(validateTransaction(tx)){
        mempool.push_back(tx);
        saveMempool(); // 🔥 IMPORTANTE
        std::cout << "TX OK\n";
    } else {
        std::cout << "TX FAIL\n";
    }
}

void Blockchain::mineBlock(const std::string& miner){

    Block b;
    b.index = chain.size();
    b.prevHash = chain.back().hash;

    std::stringstream ss;

    ss << "SYSTEM->" << miner << ":250\n";

    for(auto &tx : mempool){
        ss << tx.from << "->" << tx.to << ":" << tx.amount << "\n";
    }

    b.data = ss.str();
    b.hash = sha256(b.data + b.prevHash);

    chain.push_back(b);

    mempool.clear();
    saveMempool(); // 🔥 limpa arquivo também

    save();

    std::cout << "Bloco " << b.index << " minerado\n";
}

int Blockchain::getBalance(const std::string& addr){

    int balance = 0;

    for(auto &b : chain){

        std::stringstream ss(b.data);
        std::string line;

        while(std::getline(ss, line)){

            if(line.find("SYSTEM->") != std::string::npos){

                size_t p1 = line.find("->") + 2;
                size_t p2 = line.find(":");

                std::string to = line.substr(p1, p2 - p1);
                int amount = std::stoi(line.substr(p2 + 1));

                if(to == addr)
                    balance += amount;
            }

            else if(line.find("->") != std::string::npos){

                size_t p1 = line.find("->");
                size_t p2 = line.find(":");

                std::string from = line.substr(0, p1);
                std::string to = line.substr(p1 + 2, p2 - (p1 + 2));
                int amount = std::stoi(line.substr(p2 + 1));

                if(to == addr)
                    balance += amount;

                if(from == addr)
                    balance -= amount;
            }
        }
    }

    return balance;
}

void Blockchain::save(){

    std::ofstream f("chain.txt");

    for(auto &b : chain){
        f << b.index << "|" << b.data << "|" << b.prevHash << "|" << b.hash << "\n";
    }
}

void Blockchain::load(){

    std::ifstream f("chain.txt");

    if(!f) return;

    std::string l;

    while(getline(f, l)){

        Block b;

        size_t p1 = l.find("|");
        size_t p2 = l.find("|", p1+1);
        size_t p3 = l.find("|", p2+1);

        b.index = stoi(l.substr(0, p1));
        b.data = l.substr(p1+1, p2-p1-1);
        b.prevHash = l.substr(p2+1, p3-p2-1);
        b.hash = l.substr(p3+1);

        chain.push_back(b);
    }
}

// 🔥 NOVO
void Blockchain::saveMempool(){

    std::ofstream f("mempool.txt");

    for(auto &tx : mempool){
        f << tx.from << "|" << tx.to << "|" << tx.amount << "|" << tx.signature << "\n";
    }
}

// 🔥 NOVO
void Blockchain::loadMempool(){

    std::ifstream f("mempool.txt");
    if(!f) return;

    std::string line;

    while(std::getline(f, line)){
        Transaction tx;

        size_t p1 = line.find("|");
        size_t p2 = line.find("|", p1+1);
        size_t p3 = line.find("|", p2+1);

        tx.from = line.substr(0, p1);
        tx.to = line.substr(p1+1, p2-p1-1);
        tx.amount = stoi(line.substr(p2+1, p3-p2-1));
        tx.signature = line.substr(p3+1);

        mempool.push_back(tx);
    }
}

bool Blockchain::isValid(){

    for(size_t i = 1; i < chain.size(); i++){
        if(chain[i].prevHash != chain[i-1].hash)
            return false;
    }

    return true;
}
