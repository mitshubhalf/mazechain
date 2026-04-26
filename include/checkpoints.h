#ifndef CHECKPOINTS_H
#define CHECKPOINTS_H

#include <map>
#include <string>

namespace Checkpoints {
    // Definimos o tipo para facilitar
    typedef std::map<int, std::string> MapCheckpoints;

    // 'extern' diz ao compilador: "A lista real está em outro lugar (no .cpp)"
    extern const MapCheckpoints mapCheckpoints;

    bool CheckBlock(int height, const std::string& hash);
    int GetLastCheckpointHeight();
}

#endif