FROM arm32v7/ubuntu:focal

WORKDIR /build

ENV TZ=Europe/Belgrade
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt update
RUN apt install -y git gcc g++ pkg-config automake autotools-dev autoconf m4 zlib1g-dev \
 libtool libssl-dev devscripts debhelper libsqlite3-dev libglib2.0-dev

RUN curl https://apt.kitware.com/kitware-archive.sh > kitware-archive.sh
RUN chmod +x kitware-archive.sh
RUN ./kitware-archive.sh

RUN apt update && apt install -y cmake

COPY make_deb.sh /build
COPY *.zip /build

#CMD ["bash", "make_deb.sh"]
