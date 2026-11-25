/**
 * @file funcs.h
 * @brief Встроенные функции калькулятора: sqrt, abs, sin, cos, ln.
 */
#ifndef FUNCS_H
#define FUNCS_H

/** Результат ::func_call. */
typedef enum {
    FUNC_OK,       /**< посчитано */
    FUNC_UNKNOWN,  /**< такой функции нет */
    FUNC_DOMAIN    /**< аргумент вне области определения (sqrt(-1), ln(0)) */
} func_status;

/**
 * @brief Вызывает функцию по имени.
 * @param name имя функции
 * @param arg  аргумент
 * @param out  сюда пишется результат при ::FUNC_OK; иначе не меняется
 * @return ::FUNC_OK, ::FUNC_UNKNOWN или ::FUNC_DOMAIN
 */
func_status func_call(const char *name, double arg, double *out);

#endif
