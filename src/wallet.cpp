#include "wallet.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <cstdlib>

std::vector<std::string> wordlist = {
 "apple","banana","cat","dog","eagle","forest","gold","house",
 "ice","jungle","king","lemon","moon","night","ocean","power",
 "queen","river","sun","tree","unity","victory","world","zebra"
};

Wallet::Wallet() {}

std::string Wallet::sha256(const std::string& input) {
 unsigned char hash[SHA256_DIGEST_LENGTH];
 SHA256((unsigned char*)input.c_str(), input.size(), hash);

 std::stringstream ss;
 for(int i=0;i<SHA256_DIGEST_LENGTH;i++)
  ss<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)hash[i];

 return ss.str();
}

void Wallet::generateMnemonic() {
 for(int i=0;i<12;i++){
  mnemonic+=wordlist[rand()%wordlist.size()];
  if(i<11) mnemonic+=" ";
 }
}

void Wallet::generateFromMnemonic() {
 privateKey=sha256(mnemonic);
 publicKey=sha256(privateKey);
 address=sha256(publicKey).substr(0,40);
}

void Wallet::saveToFile(const std::string& filename){
 std::ofstream f(filename);
 f<<mnemonic<<"\n"<<privateKey<<"\n"<<publicKey<<"\n"<<address;
}

void Wallet::loadFromFile(const std::string& filename){
 std::ifstream f(filename);
 getline(f,mnemonic);
 getline(f,privateKey);
 getline(f,publicKey);
 getline(f,address);
}
