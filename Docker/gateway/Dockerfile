FROM ubuntu:16.04 as builder

RUN apt-get update && \
    apt-get dist-upgrade -y && \
    apt-get install build-essential gcc-multilib g++-multilib cmake wget python -y  && \
    apt-get clean autoclean && \
    apt-get autoremove -y && \
    wget -O /tmp/conan.deb -L https://github.com/conan-io/conan/releases/download/1.0.4/conan-ubuntu-64_1_0_4.deb && \
    dpkg -i /tmp/conan.deb

COPY    conan-profile /root/.conan/profiles/default

RUN     mkdir -p /gateway
WORKDIR /gateway
COPY    gateway.tar.gz .
RUN     tar -zxvf gateway.tar.gz
RUN     ./configure.sh

WORKDIR /gateway/out
RUN     make all -j6
RUN     ./tests

# WolkGateway
FROM ubuntu:16.04

COPY    VERSION .

RUN     mkdir -p /opt/wolkgateway
COPY    --from=builder /gateway/out/*.so* /opt/wolkgateway/
COPY    --from=builder /gateway/out/gatewayConfiguration.json /opt/wolkgateway/gatewayConfiguration.json
COPY    --from=builder /gateway/out/ca.crt /opt/wolkgateway/ca.crt
COPY    --from=builder /gateway/out/WolkGatewayApp /opt/wolkgateway/WolkGatewayApp
RUN     chmod u+x /opt/wolkgateway/WolkGatewayApp

COPY    gateway_entrypoint.sh /gateway_entrypoint.sh
RUN     chmod u+x gateway_entrypoint.sh

ENTRYPOINT [ "/gateway_entrypoint.sh" ]
