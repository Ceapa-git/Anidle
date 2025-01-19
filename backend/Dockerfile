FROM archlinux:latest

RUN pacman-key --init && \
    pacman-key --populate archlinux && \
    pacman -Syu --noconfirm

RUN pacman -Sy --noconfirm cmake gcc make

WORKDIR /app

COPY . .

ARG TOKEN
ENV TOKEN=$TOKEN

RUN mkdir build && cd build && cmake .. && make

CMD ["./build/backend", "-l"]

