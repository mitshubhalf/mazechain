#ifndef MINING_UTILS_H
#define MINING_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

typedef struct {
    char miner_address[64];
    long extra_nonce;
    double reward;
} MinerIdentity;

// CALCULA A RECOMPENSA (Sincronizado com seu sistema atual)
// Adicionado 'static inline' para evitar erro de múltiplas definições na linkagem
static inline double calculate_mining_reward(int height) {
    if (height <= 1000) {
        return 2000.0; 
    } else {
        return 1000.0; // Sua regra atual para o Bloco 1001+
    }
}

// PREPARA A IDENTIDADE ÚNICA
// Adicionado 'static inline' para que cada arquivo .cpp tenha sua própria referência segura
static inline MinerIdentity prepare_miner_identity(const char* address, int height) {
    MinerIdentity id;

    // Limpa a memória para evitar lixo de processamento
    memset(id.miner_address, 0, 64);
    strncpy(id.miner_address, address, 63);

    // ExtraNonce para garantir que seu trabalho seja único
    srand(time(NULL) + height);
    id.extra_nonce = rand() % 9000000 + 1000000; 

    id.reward = calculate_mining_reward(height);
    return id;
}

#endif