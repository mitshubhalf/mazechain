cmake_minimum_required(VERSION 3.10)
project(MazeChain)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Procura a OpenSSL
find_package(OpenSSL REQUIRED)

# 2. Configura diretórios de inclusão
include_directories(include)
include_directories(api)
include_directories(${OPENSSL_INCLUDE_DIR})

# 3. Lista de arquivos fonte (Limpa de caracteres invisíveis)
set(SOURCES 
    api/main.cpp 
    src/blockchain.cpp 
    src/block.cpp
    src/transaction.cpp
    src/utxo.cpp
    src/storage.cpp 
    src/wallet.cpp
    src/crypto.cpp 
)

#Cria o executável
add_executable(mazechain_node ${SOURCES})

# 4. Liga as bibliotecas (Target Link Libraries)
# Usar OpenSSL::SSL e OpenSSL::Crypto é o modo moderno e mais seguro
target_link_libraries(mazechain_node 
    PRIVATE 
    OpenSSL::SSL
    OpenSSL::Crypto 
    Threads::Threads
)

# 5. Adiciona busca por Threads (necessário para pthread de forma portável)
find_package(Threads REQUIRED)
target_link_libraries(mazechain_node PRIVATE Threads::Threads)
