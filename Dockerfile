FROM ubuntu:22.04

# Instala o compilador e bibliotecas necessárias
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    wget \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Cria a pasta para os arquivos do Crow
RUN mkdir -p include

# Baixa a biblioteca Crow
RUN wget https://github.com/CrowCpp/Crow/releases/download/v1.0+5/crow_all.h -O include/crow_all.h

# Compila a sua API
RUN g++ src/blockchain.cpp src/storage.cpp src/wallet.cpp api/main.cpp -Iinclude -lssl -lcrypto -lpthread -o mazechain_api

# Define a porta que o Render exige (10000 é o padrão do Render)
ENV PORT=10000
EXPOSE 10000

# Roda o servidor
CMD ["./mazechain_api"]
