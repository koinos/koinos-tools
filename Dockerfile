FROM alpine:latest as builder

RUN apk update && \
    apk add  \
        gcc \
        g++ \
        ccache \
        musl-dev \
        linux-headers \
        libgmpxx \
        cmake \
        make \
        git \
        perl \
        python3 \
        py3-pip \
        py3-setuptools && \
    pip3 install --user dataclasses_json Jinja2 importlib_resources pluginbase gitpython

ADD . /koinos-tools
WORKDIR /koinos-tools

ENV CC=/usr/lib/ccache/bin/gcc
ENV CXX=/usr/lib/ccache/bin/g++

RUN mkdir -p /koinos-tools/.ccache && \
    ln -s /koinos-tools/.ccache $HOME/.ccache && \
    git submodule update --init --recursive && \
    cmake -DCMAKE_BUILD_TYPE=Release . && \
    cmake --build . --config Release --parallel

FROM alpine:latest
RUN apk update && \
    apk add \
        musl \
        libstdc++
COPY --from=builder /koinos-tools/programs/koinos_contract_uploader/koinos_contract_uploader /usr/local/bin
COPY --from=builder /koinos-tools/programs/koinos_get_dev_key/koinos_get_dev_key /usr/local/bin
COPY --from=builder /koinos-tools/programs/koinos_transaction_signer/koinos_transaction_signer /usr/local/bin
COPY --from=builder /koinos-tools/programs/koinos_object_serializer/koinos_obj_serializer /usr/local/bin
