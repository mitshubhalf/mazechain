FROM ubuntu:22.04

# Evita perguntas interativas durante a instalação
ENV DEBIAN_FRONTEND=noninteractive

# Instala as dependências:
# Adicionamos libcurl4-openssl-dev para a comunicação entre nós
RUN apt-get update && apt-get install -y \
    g++ \
    make \
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

# Cria a pasta data para salvar a blockchain e o mempool
RUN mkdir -p data

# Compila a API com suporte a todas as bibliotecas necessárias
# Adicionamos -lcurl no final para o sistema de nós
RUN g++ src/blockchain.cpp src/storage.cpp src/wallet.cpp api/main.cpp \
    -Iinclude -lssl -lcrypto -lpthread -lcurl -o mazechain_api

# Define a porta do Render
ENV PORT=10000
EXPOSE 10000

# Comando para rodar a API
CMD ["./mazechain_api"]
