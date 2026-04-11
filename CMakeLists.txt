cmake_minimum_required(VERSION 3.10)
project(MazeChain)

set(CMAKE_CXX_STANDARD 17)

# Procura a OpenSSL para os cálculos de SHA256
find_package(OpenSSL REQUIRED)

# Configura diretórios de inclusão
include_directories(include)
include_directories(${OPENSSL_INCLUDE_DIR})

# Lista de todos os seus arquivos fonte
set(SOURCES 
    api/main.cpp 
    src/blockchain.cpp 
    src/storage.cpp 
    src/wallet.cpp
)

add_executable(mazechain_node ${SOURCES})

# Liga as bibliotecas necessárias
target_link_libraries(mazechain_node ${OPENSSL_LIBRARIES} pthread)
