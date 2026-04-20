#ifndef DB_INTEGRITY_H
#define DB_INTEGRITY_H

#include <string>

namespace DBIntegrity {
    // Gera um código de segurança baseado no conteúdo do arquivo
    std::string CalculateFileHash(const std::string& filename);
    
    // Verifica se o arquivo foi mexido fora do programa
    bool VerifyIntegrity(const std::string& dataFile, const std::string& hashFile);
    
    // Salva o novo hash após uma mineração bem sucedida
    void UpdateHash(const std::string& dataFile, const std::string& hashFile);
}

#endif
