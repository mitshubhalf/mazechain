cmake_minimum_required(VERSION 3.10)
project(MazeChain)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Procura as dependências necessárias (Removi o CURL daqui)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# 2. Configura diretórios de inclusão (Removi CURL_INCLUDE_DIRS)
include_directories(include)
include_directories(api)
include_directories(${OPENSSL_INCLUDE_DIR})

# 3. Lista de arquivos fonte TOTAL
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
    # Se você não tiver os arquivos abaixo fisicamente, 
    # remova-os para não dar erro de "File not found"
    # src/checkpoints.cpp
    # src/validation.cpp
    # src/policy.cpp
    # src/mempool_limite.cpp
    # src/db_integrity.cpp
    # src/script.cpp
)

# 4. Cria o executável
add_executable(mazechain_node ${SOURCES})

# 5. Liga as bibliotecas (Removi ${CURL_LIBRARIES})
target_link_libraries(mazechain_node 
    PRIVATE 
    OpenSSL::SSL
    OpenSSL::Crypto 
    Threads::Threads
)

# 6. Configurações adicionais
if(NOT MSVC)
    target_compile_options(mazechain_node PRIVATE -Wall -Wextra -O3 -pthread)
endif()

add_definitions(-DMAZECHAIN_VERSION="2.1.1-PROD")