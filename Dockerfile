# syntax=docker/dockerfile:1
FROM devkitpro/devkitarm
COPY . .
RUN wget https://github.com/WebAssembly/wabt/releases/download/1.0.30/wabt-1.0.30-ubuntu.tar.gz && \
    tar xzf wabt-1.0.30-ubuntu.tar.gz
ENV PATH=/wabt-1.0.30/bin:$PATH
WORKDIR /
