FROM arm64v8/debian:buster

WORKDIR /build

ENV TZ=Europe/Belgrade
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt update
RUN apt install -y git gcc g++ pkg-config automake autotools-dev autoconf m4 zlib1g-dev \
 libtool libssl-dev devscripts debhelper libsqlite3-dev libglib2.0-dev

RUN apt install -y software-properties-common
RUN curl https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null

RUN ls /etc/apt/trusted.gpg.d/
RUN cat /etc/apt/trusted.gpg.d/kitware.gpg

RUN apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'

RUN apt update && apt install -y cmake
RUN cmake --version

COPY make_deb.sh /build
COPY *.zip /build

#CMD ["bash", "make_deb.sh"]
