#ifndef DIFFICULTY_H
#define DIFFICULTY_H

#include <iostream>
#include <algorithm>

/**
 * MOTOR DE DIFICULDADE MAZECHAIN
 * Regras: 
 * - A partir de 10k: Mínimo 10 min por bloco.
 * - A partir de 20k: Ajuste estilo BTC (Janela 100, sem teto fixo de 8).
 */
namespace Difficulty {

    inline int calculate_next_difficulty(int current_height, int current_diff, long last_ts, long start_ts) {

        int floor = 4;
        long target_block_time = 60; // 1 minuto inicial
        int interval = 10;
        bool btc_logic = false;

        // --- DEFINIÇÃO DAS ERAS ---

        if (current_height <= 5000) {
            return 4; // Era Gênesis: Fixa
        }
        else if (current_height <= 10000) {
            floor = 5;
            target_block_time = 60; // 1 minuto
            interval = 10;
        }
        else if (current_height <= 20000) {
            floor = 7;
            target_block_time = 600; // Mínimo 10 minutos
            interval = 10;
        }
        else {
            floor = 8;
            target_block_time = 600; // Mantém 10 minutos
            interval = 100;         // Janela maior (BTC Style)
            btc_logic = true;
        }

        // Só recalcula a cada 'interval' blocos
        if (current_height % interval != 0) {
            return std::max(current_diff, floor);
        }

        long actual_time = last_ts - start_ts;
        long expected_time = interval * target_block_time;

        int next_diff = current_diff;

        // --- LÓGICA DE AJUSTE ESTILO BTC ---
        if (actual_time < expected_time) {
            next_diff++; 
        } 
        else if (actual_time > expected_time) {
            next_diff--;
        }

        // --- SEGURANÇA DE PISO ---
        if (next_diff < floor) next_diff = floor;

        if (btc_logic) {
            std::cout << "🌐 [MAZE-BTC] Altura: " << current_height 
                      << " | Tempo Real: " << actual_time << "s"
                      << " | Alvo: " << expected_time << "s"
                      << " | Nova Diff: " << next_diff << std::endl;
        }

        return next_diff;
    }
}

#endif