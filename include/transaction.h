#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>
#include <vector>

/**
 * @struct TxIn
 * Representa uma entrada de transação (de onde vem a moeda).
 */
struct TxIn {
    std::string txid; // ID da transação anterior
    int index;        // Índice da saída na transação anterior
};

/**
 * @struct TxOut
 * Representa uma saída de transação (para onde vai a moeda).
 */
struct TxOut {
    std::string address; // Endereço do destino
    double amount;       // Quantia enviada
};

/**
 * @struct Transaction
 * Estrutura principal de uma transação na MazeChain.
 */
struct Transaction {
    std::string id;             // O Hash ID da transação (o que o main.cpp chama de .hash)
    std::string hash;           // Alias para ID para compatibilidade com o compilador
    std::string signature;      // Assinatura digital (ECDSA/Secp256k1)
    std::string publicKey;      // Chave pública do remetente

    std::string sender;         // Endereço de quem envia (Facilitador para o main.cpp)
    std::string receiver;       // Endereço de quem recebe
    double amount;              // Valor total
    long timestamp;             // Momento da criação

    std::vector<TxOut> vout;    // Lista de saídas
    std::vector<TxIn> vin;      // Lista de entradas (UTXO)

    // --- CONSTRUTORES ---

    /**
     * Construtor padrão (vazio)
     */
    Transaction(); 

    /**
     * Construtor para envio simples (usado pelo comando 'send' do main.cpp)
     * @param s Endereço do Remetente
     * @param r Endereço do Destinatário
     * @param a Quantia
     */
    Transaction(std::string s, std::string r, double a);

    // --- FUNÇÕES DE LÓGICA ---

    /**
     * Calcula o hash SHA-256 da transação para gerar o ID único.
     */
    std::string calculateHash() const;

    /**
     * Assina a transação usando a chave privada do remetente.
     * @param privKey Chave privada em formato string (Hex)
     */
    void sign(std::string privKey);

    /**
     * Verifica se a assinatura da transação é válida.
     * Usa o txid (que NÃO inclui a assinatura) como dado verificado.
     * Padrão anti-maleabilidade: terceiros não podem alterar assinatura e mudar txid.
     */
    bool verifySignature() const;

    /**
     * Witness ID (wtxid) — inclui a assinatura, como SegWit no Bitcoin.
     * txid  (id)        = hash(vout + publicKey)         — não inclui assinatura
     * witnessId()       = hash(txid + signature + pubkey) — inclui assinatura
     * Use witnessId() para rastreamento interno; use id para referência pública.
     */
    std::string witnessId() const;
};

#endif