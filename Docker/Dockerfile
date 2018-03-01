FROM alpine:3.7

ADD VERSION .

RUN apk --no-cache add mosquitto
COPY mosquitto.conf /etc/mosquitto/mosquitto.conf

EXPOSE 1883
ENV PATH /usr/sbin:$PATH
ENTRYPOINT ["/usr/sbin/mosquitto"]

