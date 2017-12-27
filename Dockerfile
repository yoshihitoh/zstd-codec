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

ADD ./zstd /emscripten/zstd

WORKDIR /emscripten/zstd
RUN touch lib/libzstd.so && touch lib/libzstd.a && \
    rm lib/libzstd.so* lib/libzstd.a
RUN bash --login -c "emmake make clean && emmake make -j$(nproc)"
RUN mkdir -p /emscripten/lib && cp lib/libzstd.so /emscripten/lib/libzstd.bc

WORKDIR /emscripten
ADD ./build/prebuild-libc.cc /emscripten
RUN bash --login -c \
    "em++ --bind -std=c++1z -o prebuild-libc.js prebuild-libc.cc -s DEMANGLE_SUPPORT=1 && node prebuild-libc.js && rm prebuild-libc.js"

VOLUME /emscripten/artifacts
VOLUME /emscripten/build
VOLUME /emscripten/src
WORKDIR /emscripten
CMD ["/bin/bash", "--login", "build/build_zstd_binding.sh"]
