FROM arm64v8/debian:buster

WORKDIR /build

RUN apt-get update \
    && apt-get install -y git gcc g++ cmake \
    && apt-get install -y automake autotools-dev autoconf m4 zlib1g-dev cmake libtool libssl-dev libpoco-dev \
    && apt-get install -y devscripts debhelper

COPY make_deb.sh /build

#CMD ["bash", "make_deb.sh"]
