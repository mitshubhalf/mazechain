#include "../include/db_integrity.h"
#include "../include/crypto.h"
#include <fstream>
#include <sstream>

namespace DBIntegrity {
    std::string CalculateFileHash(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) return "";
        
        std::stringstream ss;
        ss << file.rdbuf();
        return Crypto::sha256_util(ss.str());
    }

    bool VerifyIntegrity(const std::string& dataFile, const std::string& hashFile) {
        std::string currentHash = CalculateFileHash(dataFile);
        
        std::ifstream fHash(hashFile);
        std::string savedHash;
        std::getline(fHash, savedHash);
        
        return currentHash == savedHash;
    }

    void UpdateHash(const std::string& dataFile, const std::string& hashFile) {
        std::string newHash = CalculateFileHash(dataFile);
        std::ofstream fHash(hashFile);
        fHash << newHash;
    }
}
