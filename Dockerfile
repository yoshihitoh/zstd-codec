FROM debian:stretch
LABEL maintainer "yoshihitoh <hammer.and.heart.daphne@gmail.com>"

ENV EMCC_SDK_VERSION    1.37.28
ENV ZSTD_DIR            /emscripten/zstd

# install prerequisites
RUN apt-get update
RUN apt-get install -y \
        wget git-core \
        build-essential cmake python nodejs openjdk-8-jre-headless 

# install emsdk
WORKDIR /emscripten
RUN wget https://s3.amazonaws.com/mozilla-games/emscripten/releases/emsdk-portable.tar.gz && \
    tar xzvf emsdk-portable.tar.gz && \
    rm emsdk-portable.tar.gz

RUN cd emsdk-portable && \
    ./emsdk update && \
    ./emsdk install sdk-${EMCC_SDK_VERSION}-64bit && \
    ./emsdk activate sdk-${EMCC_SDK_VERSION}-64bit && \
    echo "source $PWD/emsdk_env.sh" >> ~/.bashrc

# warmup emsdk
WORKDIR /emscripten
ADD ./docker-files/prebuild-libc.cc /emscripten
RUN ls -lah /emscripten && bash --login -c \
    "em++ --bind -std=c++1z -o prebuild-libc.js prebuild-libc.cc -s DEMANGLE_SUPPORT=1 && node prebuild-libc.js && rm prebuild-libc.js" && \
    rm prebuild-libc.*

# build zstd library
COPY ./cpp/zstd ${ZSTD_DIR}
WORKDIR ${ZSTD_DIR}
RUN bash --login -c "make clean && emmake make -j$(nproc)"
RUN mkdir -p /emscripten/lib && cp lib/libzstd.so /emscripten/lib/libzstd.bc

# install premake5
WORKDIR /emscripten
RUN wget https://github.com/premake/premake-core/releases/download/v5.0.0-alpha12/premake-5.0.0-alpha12-linux.tar.gz && \
    tar xvf premake-5.0.0-alpha12-linux.tar.gz && \
    rm premake-5.0.0-alpha12-linux.tar.gz && \
    mv premake5 /usr/local/bin

VOLUME /emscripten/build
VOLUME /emscripten/src
WORKDIR /emscripten

CMD ["/bin/bash"]
