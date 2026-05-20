FROM ubuntu:latest

RUN apt-get update && apt-get install -y \
    sudo \
    build-essential \
    python3 \
    cmake \
    check \
    libelf-dev \
    libzstd-dev

WORKDIR /app

COPY . /app

CMD cd test_check && make alltests
