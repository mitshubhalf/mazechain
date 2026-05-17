#ifndef DIFFICULTY_H
#define DIFFICULTY_H

#include <iostream>
#include <algorithm>

namespace Difficulty {

    /**
     * MAZECHAIN - Lógica de Ajuste de Dificuldade
     * Esta função gerencia as Eras da moeda e garante transições suaves entre blocos.
     */
    inline int calculate_next_difficulty(int current_height, int current_diff, long last_ts, long start_ts) {

        int floor = 4;
        long target_block_time = 60; // 1 minuto inicial (Era Gênesis e Fundação)
        int interval = 10;
        bool btc_logic = false;

        // --- DEFINIÇÃO DAS ERAS (SINCRONIZADA COM O MOTOR PRINCIPAL) ---

        // 1. Era Gênesis/Fundação: Do bloco 0 até o 5000.
        // O bloco 5000 ainda pertence a esta Era para evitar erros de validação.
        if (current_height <= 5000) { 
            return 4; 
        }

        // 2. Era de Transição Suave: Do bloco 5001 ao 10000.
        else if (current_height > 5000 && current_height <= 10000) {
            floor = 5;
            target_block_time = 60;
            interval = 10;
        }

        // 3. Era de Expansão: Do bloco 10001 ao 20000.
        else if (current_height <= 20000) {
            floor = 7;
            target_block_time = 600; // 10 minutos (Padrão de mercado)
            interval = 10;
        }

        // 4. Era Madura (BTC Logic): Acima de 20000.
        else {
            floor = 8;
            target_block_time = 600;
            interval = 100;
            btc_logic = true;
        }

        // --- LOGICA DE AJUSTE DINÂMICO ---

        // IMPORTANTE: Se não atingimos o intervalo de recalcular (ex: bloco 5002, 5003...),
        // mantemos a dificuldade atual, mas nunca permitimos que ela seja menor que o piso da Era.
        if (current_height % interval != 0) {
            return std::max(current_diff, floor);
        }

        long actual_time = last_ts - start_ts;
        long expected_time = interval * target_block_time;

        int next_diff = current_diff;

        // Ajuste baseado no tempo de mineração real vs esperado:
        // Se a rede está minerando rápido demais (actual < expected), aumenta a dificuldade (++).
        if (actual_time < expected_time) {
            next_diff++; 
        } 
        // Se a rede está lenta demais, diminui a dificuldade (--).
        else if (actual_time > expected_time) {
            next_diff--;
        }

        // --- SEGURANÇA DE PISO ---
        // Impede que ataques de "hashrate drop" joguem a dificuldade para zero.
        // A dificuldade mínima sempre será o 'floor' da Era atual.
        if (next_diff < floor) {
            next_diff = floor;
        }

        // Logs de Debug para monitoramento da rede (BTC logic ou a cada 100 blocos)
        if (btc_logic || current_height % 100 == 0) {
            std::cout << "🌐 [MAZECHAIN] Altura: " << current_height 
                      << " | Tempo Real: " << actual_time << "s"
                      << " | Alvo: " << expected_time << "s"
                      << " | Nova Diff: " << next_diff << " | Piso: " << floor << std::endl;
        }

        return next_diff;
    }
}

#endif