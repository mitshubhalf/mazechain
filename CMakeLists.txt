cmake_minimum_required(VERSION 3.10)
project(MazeChain)

set(CMAKE_CXX_STANDARD 17)

# 1. Procura a OpenSSL
find_package(OpenSSL REQUIRED)

# 2. Configura diretórios de inclusão
include_directories(include)
include_directories(api) # Adicionado para garantir que o crow_all.h seja achado
include_directories(${OPENSSL_INCLUDE_DIR})

# 3. Lista COMPLETA de arquivos fonte
set(SOURCES 
    api/main.cpp 
    src/blockchain.cpp 
    src/block.cpp       # <-- Faltava este
    src/transaction.cpp # <-- Faltava este
    src/utxo.cpp        # <-- Faltava este
    src/storage.cpp 
    src/wallet.cpp
    src/crypto.cpp 
)

add_executable(mazechain_node ${SOURCES})

# 4. Liga as bibliotecas
target_link_libraries(mazechain_node 
    PRIVATE 
    ${OPENSSL_LIBRARIES} 
    OpenSSL::Crypto 
    pthread
)
