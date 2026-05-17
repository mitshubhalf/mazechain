#ifndef CHECKPOINTS_H
#define CHECKPOINTS_H

#include <map>
#include <string>

namespace Checkpoints {

    typedef std::map<int, std::string> MapCheckpoints;

    // Fixed genesis checkpoint — always hardcoded, never overwritten
    extern const MapCheckpoints genesisCheckpoints;

    bool   CheckBlock(int height, const std::string& hash);
    int    GetLastCheckpointHeight();
    double GetProgressFraction(int height);

    // Dynamic halving checkpoints: saved to disk after each halving block
    void AddRuntimeCheckpoint(int height, const std::string& hash);
    void SaveCheckpoints(const std::string& path);
    void LoadCheckpoints(const std::string& path);
    bool IsHalvingBlock(int height);

} // namespace Checkpoints

#endif
