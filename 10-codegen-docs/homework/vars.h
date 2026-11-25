/**
 * @file vars.h
 * @brief Таблица переменных калькулятора.
 *
 * Хранит пары «имя, значение». Имя: буква или `_`, дальше буквы, цифры и `_`,
 * не длиннее ::VARS_NAME_MAX символов. Переменных не больше ::VARS_LIMIT.
 */
#ifndef VARS_H
#define VARS_H

#include <stdbool.h>
#include <stddef.h>

/** Максимальная длина имени переменной (без завершающего нуля). */
#define VARS_NAME_MAX 31

/** Сколько переменных можно завести. */
#define VARS_LIMIT 1024

/** Таблица переменных. Устройство скрыто, работать только через функции ниже. */
typedef struct vars vars;

/** Результат ::vars_set. */
typedef enum {
    VARS_OK,        /**< значение записано */
    VARS_BAD_NAME,  /**< имя пустое, длинное или с недопустимыми символами */
    VARS_FULL,      /**< уже ::VARS_LIMIT переменных, новой места нет */
    VARS_NOMEM      /**< не хватило памяти */
} vars_status;

/**
 * @brief Создаёт пустую таблицу.
 * @return таблица или NULL, если не хватило памяти; освобождать через ::vars_free
 */
vars *vars_new(void);

/**
 * @brief Освобождает таблицу вместе со всеми записями.
 * @param table таблица или NULL (тогда ничего не происходит)
 */
void vars_free(vars *table);

/**
 * @brief Записывает значение переменной, при необходимости создаёт её.
 * @param table таблица
 * @param name  имя, строка с нулём на конце
 * @param value новое значение
 * @return ::VARS_OK или причина отказа; при отказе таблица не меняется
 */
vars_status vars_set(vars *table, const char *name, double value);

/**
 * @brief Читает значение переменной.
 * @param table таблица
 * @param name  имя
 * @param value сюда пишется значение, если переменная есть; может быть NULL
 * @return true, если переменная найдена
 */
bool vars_get(const vars *table, const char *name, double *value);

/**
 * @brief Сколько переменных сейчас в таблице.
 * @param table таблица
 * @return число записей
 */
size_t vars_count(const vars *table);

#endif
