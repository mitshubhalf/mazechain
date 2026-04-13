FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

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

RUN mkdir -p include
RUN wget https://github.com/CrowCpp/Crow/releases/download/v1.0+5/crow_all.h -O include/crow_all.h
RUN mkdir -p data

# Compilação
RUN g++ src/blockchain.cpp \
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

# --- AJUSTES PARA O RENDER ---
# Garante que o binário tem permissão de execução
RUN chmod +x mazechain_api

# O Render exige que o app responda ao host 0.0.0.0, não apenas localhost
ENV PORT=10000

# Se o seu index.html estiver na raiz do projeto, o COPY . . já o levou para /app
# Mas o C++ precisa saber onde ele está.

CMD ["./mazechain_api"]
