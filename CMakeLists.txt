cmake_minimum_required(VERSION 3.10)
project(MazeChain)

set(CMAKE_CXX_STANDARD 17)

# 1. Procura a OpenSSL (Crucial para o SHA256)
find_package(OpenSSL REQUIRED)

# 2. Configura diretórios de inclusão
include_directories(include)
include_directories(${OPENSSL_INCLUDE_DIR})

# 3. Lista de arquivos fonte (ADICIONADO src/crypto.cpp)
set(SOURCES 
    api/main.cpp 
    src/blockchain.cpp 
    src/storage.cpp 
    src/wallet.cpp
    src/crypto.cpp  # <-- Faltava este arquivo para o SHA256 funcionar
)

add_executable(mazechain_node ${SOURCES})

# 4. Liga as bibliotecas (Adicionado OpenSSL::Crypto explicitamente)
target_link_libraries(mazechain_node 
    PRIVATE 
    ${OPENSSL_LIBRARIES} 
    OpenSSL::Crypto 
    pthread
)
