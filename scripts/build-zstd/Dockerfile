FROM debian:latest
LABEL maintainer "yoshihitoh <hammer.and.heart.daphne@gmail.com>"

ENV EMCC_SDK_VERSION 1.37.26

RUN apt-get update
RUN apt-get install -y \
        wget git-core \
        build-essential cmake python nodejs openjdk-8-jre-headless 

WORKDIR /emscripten
RUN wget https://s3.amazonaws.com/mozilla-games/emscripten/releases/emsdk-portable.tar.gz && \
    tar xzvf emsdk-portable.tar.gz && \
    rm emsdk-portable.tar.gz

RUN cd emsdk-portable && \
    ./emsdk update && \
    ./emsdk install sdk-${EMCC_SDK_VERSION}-64bit && \
    ./emsdk activate sdk-${EMCC_SDK_VERSION}-64bit && \
    echo "source $PWD/emsdk_env.sh" >> ~/.bashrc

COPY hello.c /emscripten

# test emcc, and build libc.bc
RUN /bin/bash -c "source emsdk-portable/emsdk_env.sh && emcc -o hello.js hello.c && node hello.js" && \
    rm hello.c hello.js

VOLUME /emscripten/artifacts
VOLUME /emscripten/build
VOLUME /emscripten/src
VOLUME /emscripten/zstd
CMD ["/bin/bash", "--login", "build/build_zstd_binding.sh"]
