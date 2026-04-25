#ifndef SCRIPT_H
#define SCRIPT_H

#include <string>
#include <vector>

class Script {
public:
    /**
     * @brief Valida se quem está tentando gastar a moeda tem o direito (Chave Privada).
     * @return true se a assinatura for válida para a Chave Pública e mensagem fornecidas.
     */
    static bool verify(const std::string& publicKey, const std::string& signature, const std::string& message);
    
    /**
     * @brief Cria o "Cadeado Matemático" (Locking Script).
     * Define que apenas quem provar ser o detentor do 'address' poderá gastar esses MZ.
     */
    static std::string create_locking_script(const std::string& address);
};

#endif
