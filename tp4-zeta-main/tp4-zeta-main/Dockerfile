FROM docker.io/library/ubuntu:latest

RUN apt-get update && apt-get install -y \
    sudo \
    build-essential \
    python3 \
    cmake \
    check \
    libelf-dev \
    libzstd-dev \
    unzip

WORKDIR /app

COPY . /app

CMD cd test && make alltests
