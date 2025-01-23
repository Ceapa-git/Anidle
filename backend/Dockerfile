FROM ubuntu:latest

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
  cmake \
  curl \
  g++ \
  git \
  make \
  pkg-config \
  build-essential \
  libssl-dev \
  && rm -rf /var/lib/apt/lists/*

RUN curl -OL https://github.com/mongodb/mongo-cxx-driver/releases/download/r4.0.0/mongo-cxx-driver-r4.0.0.tar.gz \
  && tar -xzf mongo-cxx-driver-r4.0.0.tar.gz \
  && cd mongo-cxx-driver-r4.0.0/build \
  && apt install pkg-config \
  && cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=17 \
  && cmake --build . \
  && cmake --build . --target install

WORKDIR /app

COPY . .

ARG TOKEN
ENV TOKEN=$TOKEN

RUN mkdir build && cd build && cmake .. && make

CMD ["./build/backend", "-l"]

