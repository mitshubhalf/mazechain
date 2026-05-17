#ifndef MINING_UTILS_H
#define MINING_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>   // Necessário para std::pow
#include <atomic> 

// Flag global para controle de interrupção
extern std::atomic<bool> global_keep_running;

typedef struct {
    char miner_address[64];
    long extra_nonce;
    double reward;
} MinerIdentity;

/**
 * CALCULA A RECOMPENSA OFICIAL (Base 150 MZ + Decaimento de 8% + Sustentabilidade)
 * Implementação matemática rigorosa para garantir o teto de 20.000.000 MZ.
 */
static inline double calculate_mining_reward(int height, double current_total_supply, double reserve_balance) {
    const double MAX_SUPPLY_LIMIT = 20000000.0;
    const int interval = 10000;
    int halving_count = height / interval;

    // FASE DE SUSTENTABILIDADE PERPÉTUA (Pós-20M ou Era 64+)
    // O fornecimento novo esgotou. O minerador agora "pesca" da reserva de taxas.
    if (current_total_supply >= MAX_SUPPLY_LIMIT || halving_count >= 64) {
        if (reserve_balance > 0.0001) {
            // Retira exatos 0.01% da reserva acumulada por taxas históricas
            return reserve_balance * 0.0001; 
        }
        return 0.00000001; // Subsídio mínimo de segurança (1 mit)
    }

    // --- FASE DE EMISSÃO (MINTING) ---
    // Regra: 150 * (0.92 ^ halving_count)
    // Isso gera o decaimento suave de 8% a cada 10.000 blocos.
    double reward = 150.0 * pow(0.92, (double)halving_count);

    // Proteção de Teto: Garante que o subsídio não ultrapasse o limite de 20M
    if (current_total_supply + reward > MAX_SUPPLY_LIMIT) {
        reward = MAX_SUPPLY_LIMIT - current_total_supply;
    }

    // Garante que a recompensa nunca seja negativa ou menor que 1 satoshi/mit
    if (reward < 0.00000001) reward = 0.00000001;

    return reward;
}

/**
 * PREPARA A IDENTIDADE ÚNICA DO MINERADOR
 * Integra o Supply atual e a Reserva para exibir a recompensa correta no log de mineração.
 */
static inline MinerIdentity prepare_miner_identity(const char* address, int height, double total_supply, double reserve_fund) {
    MinerIdentity id;

    memset(id.miner_address, 0, 64);
    if (address != NULL) {
        strncpy(id.miner_address, address, 63);
    }

    // Unicidade do ExtraNonce para evitar colisões de hash (Prevenção de Duplicate Hashes)
    static std::atomic<int> counter{0};
    int unique_seed = (int)time(NULL) + height + (++counter);
    srand(unique_seed);

    id.extra_nonce = (rand() % 9000000) + 1000000; 

    // Calcula a recompensa baseada na nova regra matemática de 150 MZ e decay de 8%
    id.reward = calculate_mining_reward(height, total_supply, reserve_fund);

    return id;
}

static inline bool should_stop_mining() {
    return !global_keep_running.load();
}

#endif