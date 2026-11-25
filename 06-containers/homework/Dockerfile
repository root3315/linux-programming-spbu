# Multistage: gcc и make остаются в этапе builder, в финальный образ
# попадает только бинарник.
FROM alpine:3.20 AS builder

RUN apk add --no-cache build-base
WORKDIR /src
COPY app/ .
RUN make

FROM alpine:3.20

# Каталог для тома. Пустой том при первом подключении получает владельца
# от этого каталога, иначе он принадлежит root и nobody туда не пишет.
# Стоит выше COPY: правка исходников не заставляет выполнять его заново.
RUN mkdir /data && chown 65534:65534 /data

COPY --from=builder /src/hello /usr/local/bin/hello

# nobody — без прав на запись куда-либо, кроме /data.
USER 65534:65534

ENTRYPOINT ["/usr/local/bin/hello"]
CMD ["Docker"]
