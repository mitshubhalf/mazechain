FROM ubuntu:22.04

# Evita perguntas interativas durante a instalação
ENV DEBIAN_FRONTEND=noninteractive

# Instala as dependências necessárias
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

# Garante que a pasta include existe e baixa o Crow
RUN mkdir -p include
RUN wget https://github.com/CrowCpp/Crow/releases/download/v1.0+5/crow_all.h -O include/crow_all.h

# Cria a pasta data para salvar a blockchain
RUN mkdir -p data

# COMPILAÇÃO CORRIGIDA:
# Incluímos todos os arquivos .cpp necessários para resolver as referências
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

# Define a porta padrão
ENV PORT=10000
EXPOSE 10000

# Comando para rodar a API
CMD ["./mazechain_api"]
