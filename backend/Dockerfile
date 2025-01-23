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
  libsasl2-dev \
  ca-certificates \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /tmp
RUN curl -OL https://github.com/mongodb/mongo-c-driver/releases/download/1.23.4/mongo-c-driver-1.23.4.tar.gz \
  && tar xzf mongo-c-driver-1.23.4.tar.gz \
  && cd mongo-c-driver-1.23.4/build \
  && cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DCMAKE_BUILD_TYPE=Release .. \
  && make -j$(nproc) \
  && make install

WORKDIR /tmp
RUN curl -OL https://github.com/mongodb/mongo-cxx-driver/releases/download/r4.0.0/mongo-cxx-driver-r4.0.0.tar.gz \
  && tar -xzf mongo-cxx-driver-r4.0.0.tar.gz \
  && cd mongo-cxx-driver-r4.0.0/build \
  && apt install pkg-config \
  && cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=17 \
  && cmake --build . \
  && cmake --build . --target install

RUN ldconfig

WORKDIR /tmp
RUN git clone --branch v0.6.0 https://github.com/Thalhammer/jwt-cpp.git \
  && cd jwt-cpp \
  && cmake -B build -S . -DJWT_CPP_BUILD_EXAMPLES=OFF -DJWT_CPP_BUILD_TESTS=OFF \
  && cmake --build build \
  && cmake --install build

RUN ldconfig

WORKDIR /app

COPY . .

ARG TOKEN
ARG MONGO_URI
ARG MONGO_DB
ARG JWT_ISSUER
ENV TOKEN=$TOKEN
ENV MONGO_URI=$MONGO_URI
ENV MONGO_DB=$MONGO_DB
ENV JWT_ISSUER=$JWT_ISSUER

RUN mkdir build && cd build && cmake .. && make -j$(nproc)

CMD ["./build/backend", "-l"]

