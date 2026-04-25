#ifndef CHECKPOINTS_H
#define CHECKPOINTS_H

#include <map>
#include <string>

namespace Checkpoints {
    // Mapa de Altura -> Hash Oficial
    // Uma vez que um bloco entra aqui, a rede NUNCA aceitará um hash diferente para essa altura.
    const std::map<int, std::string> mapCheckpoints = {
        {0, "0"}, // Hash do Gênesis
        // Quando você tiver o hash real do bloco 5000, coloque-o aqui:
        // {5000, "0000xxxxx..."} 
    };

    // Verifica se o hash de um bloco recebido bate com o checkpoint oficial
    bool CheckBlock(int height, const std::string& hash);
    
    // Retorna a altura do último checkpoint cadastrado
    int GetLastCheckpointHeight();
}

#endif
