#ifndef CHECKPOINTS_H
#define CHECKPOINTS_H

#include <map>
#include <string>

namespace Checkpoints {
    // Mapa de Altura -> Hash (Você deve preencher o hash real após minerar)
    // Exemplo: {0, "hash_do_genesis"}, {1000, "hash_do_bloco_1000"}
    const std::map<int, std::string> mapCheckpoints = {
        {0, "0"} // O Gênesis é o nosso primeiro checkpoint
        // {4000, "COLOQUE_AQUI_O_HASH_DO_BLOCO_4000_QUANDO_CHEGAR_LA"}
    };

    // Verifica se um bloco em determinada altura coincide com o checkpoint
    bool CheckBlock(int height, const std::string& hash);
    
    // Retorna a altura do último checkpoint conhecido
    int GetLastCheckpointHeight();
}

#endif
