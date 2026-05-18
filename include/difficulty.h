#ifndef DIFFICULTY_H
#define DIFFICULTY_H

#include <iostream>
#include <algorithm>

namespace Difficulty {

    /**
     * MAZECHAIN Protocol v4.0 — Motor de Dificuldade Adaptativo
     *
     * Eras de Segurança:
     *   ERA GENESIS       (     0 –  9.999): piso 4, teto 6
     *   ERA EXPANSÃO      (10.000 – 19.999): piso 5, teto 7
     *   ERA CONSOLIDAÇÃO  (20.000 – 29.999): piso 8, sem teto
     *   ERA SOBERANA      (30.000 +        ): piso 9, sem teto
     *
     * Ajuste dinâmico a cada 100 blocos baseado no tempo médio real.
     * Jamais cai abaixo do piso da Era vigente.
     */
    inline int calculate_next_difficulty(int next_height,
                                         int current_diff,
                                         long last_ts,
                                         long start_ts)
    {
        const long target_block_time = 600; // 10 minutos por bloco
        const int  window = 100;

        // ── Determina piso e teto da Era atual ──────────────────────────────
        int era_floor, era_cap;
        if (next_height < 10000) {
            era_floor = 4; era_cap = 6;      // Era Genesis
        } else if (next_height < 20000) {
            era_floor = 5; era_cap = 7;      // Era Expansão
        } else if (next_height < 30000) {
            era_floor = 8; era_cap = 9999;   // Era Consolidação (sem teto prático)
        } else {
            era_floor = 9; era_cap = 9999;   // Era Soberana (sem teto)
        }

        // Fora da janela de ajuste: mantém dificuldade dentro dos limites da Era
        if (next_height % window != 0) {
            int clamped = current_diff;
            if (clamped < era_floor) clamped = era_floor;
            if (clamped > era_cap)   clamped = era_cap;
            return clamped;
        }

        // ── Cálculo baseado no tempo real da janela ──────────────────────────
        long actual_time   = last_ts - start_ts;
        long expected_time = (long)window * target_block_time;

        int next_diff = current_diff;

        if (actual_time > 0) {
            if (actual_time < expected_time) {
                next_diff++;   // blocos chegando rápido demais → dificulta
            } else if (actual_time > expected_time) {
                next_diff--;   // blocos demorando → facilita
            }
        }

        // Aplica os limites da Era
        if (next_diff < era_floor) next_diff = era_floor;
        if (next_diff > era_cap)   next_diff = era_cap;

        std::cout << "🌐 [PROTOCOLO v4.0] Era"
                  << (next_height < 10000 ? " GENESIS" :
                      next_height < 20000 ? " EXPANSÃO" :
                      next_height < 30000 ? " CONSOLIDAÇÃO" : " SOBERANA")
                  << " | Altura: " << next_height
                  << " | Tempo Real: " << actual_time << "s"
                  << " | Alvo: " << expected_time << "s"
                  << " | Dificuldade: " << next_diff
                  << " | Piso: " << era_floor << std::endl;

        return next_diff;
    }

} // namespace Difficulty

#endif
