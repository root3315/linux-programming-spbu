# Практика 4. Найти горячую функцию

`profile/hot.c` из Chapter 07 скопировал в `~/prof`. Там две функции: наивная рекурсия `fib` и
`sum_squares(200)`, которую `main` зовёт 200 000 раз. Мой прогноз до профиля: горячая — `fib`,
экспоненциальная же. Свой файл тут один, [hot-fixed.c](hot-fixed.c).

## Общее время

```text
$ gcc -O0 -o hot-O0 hot.c
$ time ./hot-O0 34
fib(34) = 5702887
sum     = 537340000000

real	0m0.152s
user	0m0.151s
sys	0m0.001s
```

## Профиль

```text
$ gcc -O0 -pg -o hot hot.c
$ ./hot 34
fib(34) = 5702887
sum     = 537340000000
$ ls -l gmon.out
-rw-r--r-- 1 stepan stepan 2665 Nov 23 20:41 gmon.out
$ gprof -b ./hot gmon.out | head -12
Flat profile:

Each sample counts as 0.01 seconds.
  %   cumulative   self              self     total           
 time   seconds   seconds    calls  ms/call  ms/call  name    
 70.00      0.10     0.10   200000     0.00     0.00  sum_squares
 30.00      0.15     0.04        1    45.00    45.00  fib

			Call graph


granularity: each sample hit covers 4 byte(s) for 6.67% of 0.15 seconds
```

Прогноз не сбылся: первая `sum_squares`, 200 000 вызовов и 70% времени. У `fib` в колонке
`calls` стоит 1, хотя вызовов миллионы. Рекурсивные вызовы flat profile не считает, они видны в
графе вызовов:

```text
$ gprof -b -q ./hot gmon.out
...
                             18454928             fib [3]
                0.04    0.00       1/1           main [1]
[3]     30.0    0.04    0.00       1+18454928 fib [3]
                             18454928             fib [3]
```

`1+18454928`: один вызов из `main` и 18 млн из себя самой. Ещё три прогона, чтобы увидеть
разброс:

```text
$ for i in 1 2 3; do ./hot 34 >/dev/null; gprof -b -p ./hot gmon.out | sed -n 5,6p; done
 time   seconds   seconds    calls  ms/call  ms/call  name    
 69.23      0.09     0.09   200000     0.00     0.00  sum_squares
 time   seconds   seconds    calls  ms/call  ms/call  name    
 66.67      0.08     0.08   200000     0.00     0.00  sum_squares
 time   seconds   seconds    calls  ms/call  ms/call  name    
 66.67      0.08     0.08   200000     0.00     0.00  sum_squares
```

Всего 13–15 отсчётов по 10 мс, поэтому проценты прыгают. Порядок стабильный: две трети на
`sum_squares`.

## Убрать лишние вызовы

`sum_squares(200)` от `k` не зависит, все 200 000 раз считается одно и то же. В
[hot-fixed.c](hot-fixed.c) считаю один раз и умножаю:

```text
$ diff hot.c hot-fixed.c | tail -5
32,33c31
<     for (int k = 0; k < 200000; k++)
<         big += sum_squares(200);
---
>     unsigned long big = 200000UL * sum_squares(200);
$ gcc -O0 -pg -o hot-fixed-pg hot-fixed.c && ./hot-fixed-pg 34
fib(34) = 5702887
sum     = 537340000000
$ gprof -b -p ./hot-fixed-pg gmon.out | head -7
Flat profile:

Each sample counts as 0.01 seconds.
  %   cumulative   self              self     total           
 time   seconds   seconds    calls  ms/call  ms/call  name    
100.00      0.03     0.03        1    30.00    30.00  fib
  0.00      0.03     0.00        1     0.00     0.00  sum_squares
```

Результат тот же, `sum_squares` вызывается один раз, теперь всё время у `fib`.

## Сравнение

```text
$ gcc -O2 -o hot-O2 hot.c
$ gcc -O0 -o hot-fixed-O0 hot-fixed.c
$ gcc -O2 -o hot-fixed hot-fixed.c
$ TIMEFORMAT="%R s"
$ for p in hot-O0 hot-O2 hot-fixed-O0 hot-fixed; do echo -n "$p: "; time ./$p 34 >/dev/null; done
hot-O0: 0.136 s
hot-O2: 0.018 s
hot-fixed-O0: 0.055 s
hot-fixed: 0.018 s
```

| | `-O0` | `-O2` |
|---|---|---|
| `hot.c` | 0.136 с | 0.018 с |
| `hot-fixed.c` | 0.055 с | 0.018 с |

При `-O0` правка дала 2.5 раза. А `-O2` на исходном коде дал 7.5 раз, и после него моя
правка уже ничего не добавляет. На слайде наоборот, алгоритм выигрывает у флага. Почему так,
показал профиль с `-O2`:

```text
$ gcc -O2 -pg -o hot-O2-pg hot.c && ./hot-O2-pg 34 >/dev/null
$ gprof -b -p ./hot-O2-pg gmon.out | head -8
Flat profile:

Each sample counts as 0.01 seconds.
  %   cumulative   self              self     total           
 time   seconds   seconds    calls  ms/call  ms/call  name    
100.00      0.01     0.01        1    10.00    10.00  fib
$ objdump -d hot-O2 | grep -c "call.*sum_squares"
0
$ nm hot-O2 | grep -i "fib\|sum_sq"
00000000000011e0 t fib
```

`sum_squares` в `-O2` нет вообще: функция `static`, без побочных эффектов, с константным
аргументом. Компилятор встроил её и сам вынес из цикла, то есть сделал мою оптимизацию за
меня. Отсюда и ответ на вопрос со слайда: профиль с `-O2` показывает уже другой код, функции
там могут быть встроены и пропасть из списка.

В моём случае флаг дал больше, потому что лишняя работа была очевидна и компилятору. Правка
алгоритма, которую компилятор не сделает, — это `fib`. Её время растёт в φ ≈ 1.6 раза на
каждую единицу `n`:

```text
$ for n in 34 37 40; do t=$( { time ./hot-fixed $n >/dev/null; } 2>&1 ); echo "hot-fixed $n: $t"; done
hot-fixed 34: 0.027 s
hot-fixed 37: 0.101 s
hot-fixed 40: 0.483 s
```

Тут `-O2` уже не спасает, нужен цикл вместо рекурсии. `perf` в контейнере не ставил: на
занятии он только по выводу, ему нужны права на `perf_event_paranoid`.

## Что понял

Интуиция ошиблась: я ставил на рекурсию, а 70% ела простая функция в цикле. Сравнивать надо
одинаковые флаги: замер на `-O0` и замер на `-O2` отвечают на разные вопросы. И профиль
снимать на той сборке, которую реально оптимизируешь, с `-O2` картина другая.
