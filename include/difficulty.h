#ifndef DIFFICULTY_H
#define DIFFICULTY_H

#include <iostream>
#include <algorithm>

namespace Difficulty {

    /**
     * MAZECHAIN - Lógica de Ajuste de Dificuldade (Era Dinâmica Avançada)
     * Usada APENAS para blocos 20.000 em diante.
     * Eras Inicial (0-9999, diff=4) e de Transição (10000-19999, diff=5)
     * são tratadas diretamente por adjustDifficulty() em blockchain.cpp.
     */
    inline int calculate_next_difficulty(int current_height, int current_diff, long last_ts, long start_ts) {

        // Era Dinâmica Avançada (20.000+): piso mínimo 6, janela de 100 blocos
        int floor = 6;
        long target_block_time = 600; // 10 minutos
        int interval = 100;

        if (current_height % interval != 0) {
            return std::max(current_diff, floor);
        }

        long actual_time   = last_ts - start_ts;
        long expected_time = (long)interval * target_block_time;

        int next_diff = current_diff;

        if (actual_time < expected_time) {
            next_diff++;
        } else if (actual_time > expected_time) {
            next_diff--;
        }

        if (next_diff < floor) next_diff = floor;

        std::cout << "🌐 [MAZECHAIN] Altura: " << current_height
                  << " | Tempo Real: " << actual_time << "s"
                  << " | Alvo: " << expected_time << "s"
                  << " | Nova Diff: " << next_diff << " | Piso: " << floor << std::endl;

        return next_diff;
    }
}

#endif
