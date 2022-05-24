FROM amd64/debian:buster

WORKDIR /root

RUN apt update
RUN apt install -y libglib2.0-0 libsqlite3-0 libssl1.1

COPY *.deb /root
RUN dpkg -i *.deb
RUN rm *.deb

CMD ["WolkGatewayApp", "/etc/wolkGateway/gatewayConfiguration.json", "INFO"]
