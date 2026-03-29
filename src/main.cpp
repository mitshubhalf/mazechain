#include <iostream>
#include "wallet.h"
#include "blockchain.h"
#include "transaction.h"

std::string signData(const std::string&, const std::string&);

int main(int argc,char* argv[]){

    if(argc<2){
        std::cout<<"createwallet | mine | balance | send\n";
        return 0;
    }

    std::string cmd=argv[1];

    if(cmd=="createwallet"){
        Wallet w;
        w.generateMnemonic();
        w.generateFromMnemonic();
        w.saveToFile("wallet.dat");

        std::cout<<"Address: "<<w.address<<"\n";
    }

    if(cmd=="mine"){
        Wallet w; w.loadFromFile("wallet.dat");
        Blockchain bc;
        bc.mineBlock(w.address);
    }

    if(cmd=="balance"){
        Wallet w; w.loadFromFile("wallet.dat");
        Blockchain bc;
        std::cout<<"Balance: "<<bc.getBalance(w.address)<<" MC\n";
    }

    if(cmd=="send"){

        if(argc < 4){
            std::cout<<"Uso: send ADDRESS AMOUNT\n";
            return 0;
        }

        Wallet w; 
        w.loadFromFile("wallet.dat");

        Transaction tx;
        tx.from = w.address;
        tx.to = argv[2];
        tx.amount = std::stoi(argv[3]);

        tx.publicKey = w.publicKey; // 🔥 ESSENCIAL
        tx.signature = signData(tx.toString(), w.privateKey);

        Blockchain bc;
        bc.addTransaction(tx);
    }

    return 0;
}
