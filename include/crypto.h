#pragma once
#include <string>

std::string sha256(const std::string &data);

std::string generatePrivateKey();
std::string getPublicKey(const std::string &privateKey);

std::string signData(const std::string &data, const std::string &privateKey);
bool verifySignature(const std::string &data, const std::string &signature, const std::string &publicKey);
