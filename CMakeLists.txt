cmake_minimum_required(VERSION 3.10)
project(MazeChain)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Procura as dependências necessárias
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(CURL REQUIRED) # Adicionado CURL pois seu p2p.cpp usa <curl/curl.h>

# 2. Configura diretórios de inclusão
include_directories(include)
include_directories(src)      # Adicionado para resolver o erro do blockchain.h
include_directories(api)
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${CURL_INCLUDE_DIRS})

# 3. Lista de arquivos fonte TOTAL
# Removi os '#' das linhas abaixo para que o compilador inclua esses arquivos
set(SOURCES 
    api/main.cpp 
    src/blockchain.cpp 
    src/block.cpp
    src/transaction.cpp
    src/utxo.cpp
    src/storage.cpp 
    src/wallet.cpp
    src/crypto.cpp 
    src/p2p.cpp
    src/node_manager.cpp
    src/checkpoints.cpp
    src/mempool_limit.cpp
    src/db_integrity.cpp
)

# 4. Cria o executável
add_executable(mazechain_node ${SOURCES})

# 5. Liga as bibliotecas (IMPORTANTE: Adicionado CURL para o P2P)
target_link_libraries(mazechain_node 
    PRIVATE 
    OpenSSL::SSL
    OpenSSL::Crypto 
    Threads::Threads
    ${CURL_LIBRARIES} # Resolve os erros de 'undefined reference to curl_easy_init'
)

# 6. Configurações adicionais
if(NOT MSVC)
    # Adicionado -Wno-deprecated-declarations para silenciar os avisos do OpenSSL 3.0
    target_compile_options(mazechain_node PRIVATE -Wall -Wextra -O3 -pthread -Wno-deprecated-declarations)
endif()

add_definitions(-DMAZECHAIN_VERSION="3.0.0-BETA")