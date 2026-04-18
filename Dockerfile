FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Instalação de dependências essenciais
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    cmake \
    wget \
    libssl-dev \
    libboost-system-dev \
    libboost-thread-dev \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Download do Crow (Garantindo que ele vá para /app/include)
RUN mkdir -p include
RUN wget https://github.com/CrowCpp/Crow/releases/download/v1.0+5/crow_all.h -O include/crow_all.h
RUN mkdir -p data

# Compilação
# Adicionado -std=c++17 pois o Crow e o Blockchain moderno precisam
RUN g++ -O3 -std=c++17 \
        src/blockchain.cpp \
        src/block.cpp \
        src/transaction.cpp \
        src/utxo.cpp \
        src/storage.cpp \
        src/wallet.cpp \
        src/crypto.cpp \
        api/main.cpp \
    -Iinclude \
    -I. \
    -lssl -lcrypto -lpthread -lboost_system -lboost_thread -lcurl \
    -o mazechain_api

# Permissões
RUN chmod +x mazechain_api

# Porta padrão (O Render vai sobrescrever isso, mas é bom deixar)
EXPOSE 8080

# Comando de inicialização
CMD ["./mazechain_api"]
