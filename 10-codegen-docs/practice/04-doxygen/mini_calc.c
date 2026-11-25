/**
 * @file mini_calc.c
 * @brief Калькулятор без flex и bison — всё руками, в одном файле.
 *
 * Зачем этот файл: чтобы увидеть, что именно делают генераторы.
 * Здесь те же два этапа — «разбить на слова» и «разобрать по правилам»,
 * только написанные вручную. Для калькулятора это ещё терпимо;
 * для настоящего языка — сотни строк и легко ошибиться.
 *
 * Сборка: gcc -Wall -Wextra -O2 -o mini_calc mini_calc.c -lm
 * Запуск: printf '2 + 3 * 4\n' | ./mini_calc
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/** Что за слово нам встретилось. */
typedef enum {
    T_NUM,   /**< число */
    T_PLUS,  /**< + */
    T_MINUS, /**< - */
    T_STAR,  /**< * */
    T_SLASH, /**< / */
    T_POW,   /**< ^ */
    T_LP,    /**< ( */
    T_RP,    /**< ) */
    T_END,   /**< конец строки */
    T_BAD    /**< непонятный символ */
} TokenKind;

/** Одно слово и его значение. */
typedef struct {
    TokenKind kind;
    double value;
} Token;

static const char *p;   /**< где мы сейчас в строке */

/**
 * @brief Взять следующее слово из строки.
 * @return токен: число или знак
 */
static Token next_token(void)
{
    Token t = { T_END, 0 };

    while (*p == ' ' || *p == '\t')
        p++;

    if (*p == '\0' || *p == '\n')
        return t;

    if (isdigit((unsigned char)*p) || *p == '.') {
        char *end = NULL;
        t.value = strtod(p, &end);
        if (end == p) {
            t.kind = T_BAD;
            p++;
            return t;
        }
        p = end;
        t.kind = T_NUM;
        return t;
    }

    switch (*p++) {
    case '+': t.kind = T_PLUS;  break;
    case '-': t.kind = T_MINUS; break;
    case '*': t.kind = T_STAR;  break;
    case '/': t.kind = T_SLASH; break;
    case '^': t.kind = T_POW;   break;
    case '(': t.kind = T_LP;    break;
    case ')': t.kind = T_RP;    break;
    default:  t.kind = T_BAD;   break;
    }
    return t;
}

static Token current;   /**< слово, которое смотрим прямо сейчас */

static void advance(void) { current = next_token(); }

static double parse_expr(void);
static double parse_unary(void);

/**
 * @brief Разобрать «кирпичик»: число или выражение в скобках.
 */
static double parse_atom(void)
{
    if (current.kind == T_NUM) {
        double v = current.value;
        advance();
        return v;
    }
    if (current.kind == T_LP) {
        advance();
        double v = parse_expr();
        if (current.kind == T_RP)
            advance();
        return v;
    }
    return 0;
}

/**
 * @brief Степень. Правая ассоциативность: 2^3^2 это 2^(3^2).
 */
static double parse_pow(void)
{
    double base = parse_atom();

    if (current.kind == T_POW) {
        advance();
        return pow(base, parse_unary());
    }
    return base;
}

/**
 * @brief Унарный минус. Младше степени: -2^2 это -(2^2).
 */
static double parse_unary(void)
{
    if (current.kind == T_MINUS) {
        advance();
        return -parse_unary();
    }
    return parse_pow();
}

/**
 * @brief Поделить a на b без падения на нуле.
 * @param a делимое
 * @param b делитель; если 0, печатается сообщение
 * @return a / b или 0, если b равно 0
 */
double safe_div(double a, double b)
{
    if (b == 0) {
        printf("делить на ноль нельзя\n");
        return 0;
    }
    return a / b;
}

/**
 * @brief Умножение и деление — старше сложения.
 */
static double parse_term(void)
{
    double v = parse_unary();
    for (;;) {
        if (current.kind == T_STAR) {
            advance();
            v *= parse_unary();
        } else if (current.kind == T_SLASH) {
            advance();
            v = safe_div(v, parse_unary());
        } else {
            return v;
        }
    }
}

/**
 * @brief Сложение и вычитание — самый низкий приоритет.
 */
static double parse_expr(void)
{
    double v = parse_term();
    for (;;) {
        if (current.kind == T_PLUS) {
            advance();
            v += parse_term();
        } else if (current.kind == T_MINUS) {
            advance();
            v -= parse_term();
        } else {
            return v;
        }
    }
}

/**
 * @brief Читает строки со стандартного ввода и печатает значение каждой.
 * @return 0 после конца ввода
 */
int main(void)
{
    char line[256];

    printf("калькулятор без flex и bison: строка — Enter — результат\n");

    while (fgets(line, sizeof line, stdin)) {
        p = line;
        advance();
        if (current.kind == T_END) {
            printf("= пусто\n");
            continue;
        }
        double v = parse_expr();
        printf("= %g\n", v);
    }

    return 0;
}
