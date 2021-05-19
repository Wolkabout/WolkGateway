FROM amd64/debian:buster

WORKDIR /build

ENV TZ=Europe/Belgrade
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt update
RUN apt install -y git gcc g++ cmake automake autotools-dev autoconf m4 zlib1g-dev \
 cmake libtool libssl-dev devscripts debhelper

COPY make_deb.sh /build
COPY *.zip /build

#CMD ["bash", "make_deb.sh"]
