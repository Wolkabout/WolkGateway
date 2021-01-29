FROM ubuntu:bionic-20200403

WORKDIR /build

RUN apt-get update \
    && apt-get install -y git gcc g++ cmake zlib1g-dev \
    && apt-get install -y mosquitto cmake libssl-dev \
    && apt-get install -y devscripts debhelper

COPY make_deb.sh /build

#CMD ["bash", "make_deb.sh"]
