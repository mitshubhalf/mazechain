FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Instalação de dependências
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    cmake \
    wget \
    libssl-dev \
    libboost-all-dev \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Download do Crow (Header-only)
RUN mkdir -p include
RUN wget https://github.com/CrowCpp/Crow/releases/download/v1.0+5/crow_all.h -O include/crow_all.h
RUN mkdir -p data

# Compilação
# Note: Adicionei -O3 para o nó minerar mais rápido no servidor
RUN g++ -O3 src/blockchain.cpp \
        src/block.cpp \
        src/transaction.cpp \
        src/utxo.cpp \
        src/storage.cpp \
        src/wallet.cpp \
        src/crypto.cpp \
        api/main.cpp \
    -Iinclude \
    -lssl -lcrypto -lpthread -lboost_system -lboost_thread -lcurl \
    -o mazechain_api

# Permissões
RUN chmod +x mazechain_api

# --- AJUSTE DE PORTA PARA RENDER ---
# O Render define a porta automaticamente na variável de ambiente PORT.
# O seu código C++ deve ler essa variável: std::getenv("PORT")
ENV PORT=8080

# Comando de inicialização
# Usamos o formato JSON para evitar problemas com sinais do sistema
CMD ["./mazechain_api"]
