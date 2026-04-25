#ifndef P2P_H
#define P2P_H

#include <vector>
#include <string>
#include <set>

/**
 * @class P2P
 * @brief Gerencia a comunicação entre nós da rede MazeChain.
 * Responsável por propagar blocos/transações e sincronizar a blockchain.
 */
class P2P {
public:
    // Conjunto de URLs de peers conhecidos (ex: "http://192.168.1.10:18080")
    // Usamos std::set para evitar duplicatas automaticamente.
    std::set<std::string> peers;

    /**
     * @brief Adiciona um novo nó minerador à lista de contatos.
     * @param peer_url URL base do peer.
     */
    void add_peer(std::string peer_url);

    /**
     * @brief Envia um bloco recém-minerado para todos os peers da rede.
     * @param block_json Bloco serializado em formato JSON.
     */
    void broadcast_block(const std::string& block_json);

    /**
     * @brief Propaga uma nova transação recebida para os outros nós (Mempool Relay).
     * @param tx_json Transação serializada em formato JSON.
     */
    void broadcast_transaction(const std::string& tx_json);

    /**
     * @brief Solicita aos peers todos os blocos já minerados para atualizar a chain local.
     * Útil ao iniciar o nó ou após ficar offline.
     */
    void sync_with_peers();

private:
    /**
     * @brief Método auxiliar para realizar requisições POST assíncronas.
     * Garante que a rede não trave o processo de mineração.
     */
    void internal_async_post(const std::string& endpoint, const std::string& json_data);
};

#endif
