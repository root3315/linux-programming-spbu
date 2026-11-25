# Практика 3. HTTP своими руками

`http_get.c` из архива Chapter 09 (не менял), сервер из `python3`. В выводе ниже вырезал
строки `Date` и `Last-Modified`, остальное как есть.

## Сервер и запрос

```text
stepan@box:~/net-lab$ gcc -o http_get http_get.c
stepan@box:~/net-lab$ mkdir site && echo "<h1>привет</h1>" > site/index.html
stepan@box:~/net-lab$ (cd site && python3 -m http.server 8001 > ../http.log 2>&1 &)
stepan@box:~/net-lab$ ./http_get 127.0.0.1 8001 /
--- отправляю ---
GET / HTTP/1.0
Host: 127.0.0.1:8001
User-Agent: uchebnyy-klient

--- получил ---
HTTP/1.0 200 OK
Server: SimpleHTTP/0.6 Python/3.12.3
Content-type: text/html
Content-Length: 22

<h1>привет</h1>
```

Код состояния в первой строке ответа: `200 OK`. `Content-Length: 22`: это размер файла
`index.html`, `<h1>` и `</h1>` дают 9 байт, `привет` 12 байт (6 букв по 2 байта в UTF-8) и
перевод строки ещё один. Тело отделено от заголовков пустой строкой, как и запрос
заканчивается пустой строкой.

## Несуществующий файл

```text
stepan@box:~/net-lab$ ./http_get 127.0.0.1 8001 /nope
--- отправляю ---
GET /nope HTTP/1.0
Host: 127.0.0.1:8001
User-Agent: uchebnyy-klient

--- получил ---
HTTP/1.0 404 File not found
Server: SimpleHTTP/0.6 Python/3.12.3
Connection: close
Content-Type: text/html;charset=utf-8
Content-Length: 335

<!DOCTYPE HTML>
<html lang="en">
    <head>
        <meta charset="utf-8">
        <title>Error response</title>
    </head>
    <body>
        <h1>Error response</h1>
        <p>Error code: 404</p>
        <p>Message: File not found.</p>
        <p>Error code explanation: 404 - Nothing matches the given URI.</p>
    </body>
</html>
```

## Сравнение с curl

```text
stepan@box:~/net-lab$ curl -si http://127.0.0.1:8001/
HTTP/1.0 200 OK
Server: SimpleHTTP/0.6 Python/3.12.3
Content-type: text/html
Content-Length: 22

<h1>привет</h1>
```

Ответ тот же. Флаг `-s` убирает индикатор загрузки, который curl рисует в stderr, а `-i` печатает
заголовки. Запрос у curl другой: `HTTP/1.1` и свои заголовки, это видно в логе сервера:

```text
stepan@box:~/net-lab$ cat http.log
127.0.0.1 - - [...] "GET / HTTP/1.0" 200 -
127.0.0.1 - - [...] code 404, message File not found
127.0.0.1 - - [...] "GET /nope HTTP/1.0" 404 -
127.0.0.1 - - [...] "GET / HTTP/1.1" 200 -
```

## Что понял

Браузер открывает сокет, пишет `GET /путь HTTP/1.x`, несколько заголовков и пустую строку, и читает
ответ: статус, заголовки, пустая строка, тело. Всё это текст, его видно глазами. Пустая строка
в конце запроса значит «я закончил», без неё сервер ждал бы дальше. curl делает то же самое,
только аккуратнее.
