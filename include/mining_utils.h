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

// CALCULA A RECOMPENSA DEFINITIVA (Sincronizado com a regra de 400 MZ)
static inline double calculate_mining_reward(int height) {
    int interval = 10000;
    int halving_count = height / interval;

    if (halving_count >= 64) return 0.00000001;

    double reward = 400.0; // BASE INICIAL DEFINITIVA

    // FASE 1: Redução de 50% (H0 a H3)
    if (halving_count < 4) {
        for (int i = 0; i < halving_count; i++) {
            reward *= 0.5;
        }
    } 
    // FASE 2: Redução de 20% (H4 a H19)
    else if (halving_count < 20) {
        reward = 40.0; 
        for (int i = 4; i < halving_count; i++) {
            reward *= 0.80;
        }
    }
    // FASE 3: Redução de 10% (H20 a H49)
    else if (halving_count < 50) {
        reward = 1.407; 
        for (int i = 20; i <= halving_count; i++) {
            reward *= 0.90;
        }
    }
    // FASE 4: Redução de 2% (H50 a H63)
    else {
        reward = 0.060; 
        for (int i = 50; i < halving_count; i++) {
            reward *= 0.98;
        }
    }

    return reward;
}

// PREPARA A IDENTIDADE ÚNICA DO MINERADOR
static inline MinerIdentity prepare_miner_identity(const char* address, int height) {
    MinerIdentity id;

    // Limpa a memória para evitar lixo de processamento
    memset(id.miner_address, 0, 64);
    if (address != NULL) {
        strncpy(id.miner_address, address, 63);
    }

    // ExtraNonce baseado no tempo e altura para garantir unicidade do hash
    srand((unsigned int)(time(NULL) + height));
    id.extra_nonce = (rand() % 9000000) + 1000000; 

    // Atribui a recompensa calculada pela nova regra
    id.reward = calculate_mining_reward(height);

    return id;
}

#endif