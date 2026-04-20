cmake_minimum_required(VERSION 3.10)
project(MazeChain)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Procura as dependências necessárias
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# 2. Configura diretórios de inclusão
include_directories(include)
include_directories(api)
include_directories(${OPENSSL_INCLUDE_DIR})

# 3. Lista de arquivos fonte atualizada com as novas camadas de proteção
set(SOURCES 
    api/main.cpp 
    src/blockchain.cpp 
    src/block.cpp
    src/transaction.cpp
    src/utxo.cpp
    src/storage.cpp 
    src/wallet.cpp
    src/crypto.cpp 
    # Novas Camadas de Proteção
    src/checkpoints.cpp
    src/validation.cpp
    src/policy.cpp
    src/mempool_limit.cpp
    src/db_integrity.cpp
)

# 4. Cria o executável
add_executable(mazechain_node ${SOURCES})

# 5. Liga as bibliotecas (Modo Moderno do CMake)
target_link_libraries(mazechain_node 
    PRIVATE 
    OpenSSL::SSL
    OpenSSL::Crypto 
    Threads::Threads
)

# 6. Configurações adicionais de compilação (Opcional, ajuda a debugar)
if(MSVC)
    target_compile_options(mazechain_node PRIVATE /W4)
else()
    target_compile_options(mazechain_node PRIVATE -Wall -Wextra)
endif()
