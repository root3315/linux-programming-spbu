# Практика 4. compose из двух сервисов

`compose.yml` из Chapter 06: два сервиса `hello-ru` и `hello-en`, у обоих `build: .`, разница
только в `GREETING`. Работал в `~/docker-lab`, поэтому имя проекта compose — `docker-lab`.
Своих файлов нет, в шаге 4 правил `GREETING` в копии архива.

## Шаг 1. docker compose config

```text
$ docker compose config --services
hello-en
hello-ru
$ docker compose config | grep -B1 -A1 GREETING
    environment:
      GREETING: Hello
    networks:
--
    environment:
      GREETING: Привет
    networks:
```

Полный `config` показывает то же, плюс абсолютный путь контекста сборки и сеть
`docker-lab_default`, которую compose добавил сам.

## Шаг 2. docker compose up

```text
$ docker compose up
...
 Image docker-lab-hello-en Building
 Image docker-lab-hello-ru Building
 Image docker-lab-hello-en Built
 Image docker-lab-hello-ru Built
 Network docker-lab_default Creating
 Network docker-lab_default Created
 Container docker-lab-hello-en-1 Creating
 Container docker-lab-hello-ru-1 Creating
 Container docker-lab-hello-en-1 Created
 Container docker-lab-hello-ru-1 Created
Attaching to hello-en-1, hello-ru-1
 Container docker-lab-hello-ru-1 Starting
 Container docker-lab-hello-en-1 Starting
hello-ru-1  | Привет, world!
hello-ru-1  | данные из образа: этот файл попал сюда через COPY
hello-ru-1  | второй строкой — чтобы видеть, что прочитано всё
hello-en-1  | Hello, world!
hello-en-1  | данные из образа: этот файл попал сюда через COPY
hello-en-1  | второй строкой — чтобы видеть, что прочитано всё
 Container docker-lab-hello-ru-1 Started
hello-ru-1 exited with code 0
hello-en-1 exited with code 0
 Container docker-lab-hello-en-1 Started
```

Вывод обоих сервисов идёт вперемешку с префиксом имени. `up` вернул управление сам: оба
контейнера завершились.

Проверил, правда ли образ собран один раз:

```text
$ docker images --format 'table {{.Repository}}\t{{.ID}}\t{{.Size}}' | grep -E 'REPOSITORY|docker-lab'
REPOSITORY              IMAGE ID       SIZE
docker-lab-hello-ru     b64f33153ee4   12.2MB
docker-lab-hello-en     d189737a5a4e   12.2MB
$ docker image inspect --format '{{json .RootFS.Layers}}' docker-lab-hello-ru docker-lab-hello-en hello:multi | cut -c1-90
["sha256:08bc4e534116aa76b16015484b82eac51f9a593416feae9296c8a2d4bb7aa4a2","sha256:163da14
["sha256:08bc4e534116aa76b16015484b82eac51f9a593416feae9296c8a2d4bb7aa4a2","sha256:163da14
["sha256:08bc4e534116aa76b16015484b82eac51f9a593416feae9296c8a2d4bb7aa4a2","sha256:163da14
```

Не совсем. Образов два, с разными ID, но слои у них (и у `hello:multi` из практики 3)
одинаковые: вторая сборка целиком взялась из кэша, на диске слои лежат один раз. Разные
ID дают метаданные. Чтобы образ был буквально один, в ДЗ первому сервису дал `build` и
`image: hello-hw:multi`, а второму только `image`.

## Шаг 3. В фоне и статус

```text
$ docker compose up -d
 Container docker-lab-hello-en-1 Starting
 Container docker-lab-hello-ru-1 Starting
 Container docker-lab-hello-ru-1 Started
 Container docker-lab-hello-en-1 Started
$ docker compose ps -a
NAME                    IMAGE                 COMMAND                  SERVICE    CREATED          STATUS                     PORTS
docker-lab-hello-en-1   docker-lab-hello-en   "/app/hello world /d…"   hello-en   52 seconds ago   Exited (0) 5 seconds ago
docker-lab-hello-ru-1   docker-lab-hello-ru   "/app/hello world /d…"   hello-ru   52 seconds ago   Exited (0) 6 seconds ago
```

Без `-a` список пустой: `ps` по умолчанию показывает только работающие. `Exited (0)`, потому
что программа напечатала приветствие и вышла, а контейнер живёт, пока живёт PID 1.
Контейнеры не пересоздались (CREATED 52 секунды назад), их просто запустили ещё раз.

## Шаг 4. Поменять GREETING у одного сервиса

```text
$ sed -i 's/GREETING: "Hello"/GREETING: "Hi"/' compose.yml
$ grep -n GREETING compose.yml
7:      GREETING: "Привет"
12:      GREETING: "Hi"
$ docker compose up -d hello-en
 Container docker-lab-hello-en-1 Recreate
 Container docker-lab-hello-en-1 Recreated
 Container docker-lab-hello-en-1 Starting
 Container docker-lab-hello-en-1 Started
```

Конфигурация сервиса изменилась, поэтому compose пересоздал контейнер (`Recreate`), а
`hello-ru` не трогал.

## Шаг 5. Логи

```text
$ docker compose logs hello-en
hello-en-1  | Hi, world!
hello-en-1  | данные из образа: этот файл попал сюда через COPY
hello-en-1  | второй строкой — чтобы видеть, что прочитано всё
$ docker compose logs hello-ru
hello-ru-1  | Привет, world!
hello-ru-1  | данные из образа: этот файл попал сюда через COPY
hello-ru-1  | второй строкой — чтобы видеть, что прочитано всё
hello-ru-1  | Привет, world!
hello-ru-1  | данные из образа: этот файл попал сюда через COPY
hello-ru-1  | второй строкой — чтобы видеть, что прочитано всё
$ docker compose ps -a --format 'table {{.Name}}\t{{.Status}}'
NAME                    STATUS
docker-lab-hello-en-1   Exited (0) 35 seconds ago
docker-lab-hello-ru-1   Exited (0) 2 minutes ago
```

У `hello-ru` вывод дважды: это тот же контейнер, запущенный в шагах 2 и 3, логи
копятся. У `hello-en` один раз, у пересозданного контейнера история началась заново.

## Шаг 6. down

```text
$ docker compose down
 Container docker-lab-hello-en-1 Stopping
 Container docker-lab-hello-ru-1 Stopping
 Container docker-lab-hello-ru-1 Stopped
 Container docker-lab-hello-ru-1 Removing
 Container docker-lab-hello-en-1 Stopped
 Container docker-lab-hello-en-1 Removing
 Container docker-lab-hello-ru-1 Removed
 Container docker-lab-hello-en-1 Removed
 Network docker-lab_default Removing
 Network docker-lab_default Removed
$ docker compose ps -a
NAME      IMAGE     COMMAND   SERVICE   CREATED   STATUS    PORTS
$ docker images --format '{{.Repository}}:{{.Tag}} {{.Size}}' | grep docker-lab
docker-lab-hello-en:latest 12.2MB
docker-lab-hello-ru:latest 12.2MB
```

`down` удалил контейнеры и сеть, образы остались. `stop` только остановил бы контейнеры:
они остались бы в `ps -a` вместе с логами, и `up` поднял бы их снова без пересоздания.

## Тома

Пример со слайда про данные:

```text
$ docker run --rm -v demo-data:/data alpine:3.20 sh -c 'echo hi > /data/f'
$ docker run --rm -v demo-data:/data alpine:3.20 cat /data/f
hi
$ docker volume ls --filter name=demo-data
DRIVER    VOLUME NAME
local     demo-data
$ docker run --rm alpine:3.20 sh -c 'echo hi > /tmp/f'
$ docker run --rm alpine:3.20 cat /tmp/f
cat: can't open '/tmp/f': No such file or directory
$ docker volume rm demo-data
demo-data
```

Файл в томе пережил контейнер, файл в записываемом слое ушёл вместе с `--rm`.

## Что понял

Сервисы не «висят», потому что программа одноразовая: PID 1 завершился, контейнер
остановился с кодом 0. `docker compose stop` останавливает контейнеры и оставляет их, а
`down` удаляет контейнеры и сеть; образы и тома переживают оба (тома удаляет `down -v`).
