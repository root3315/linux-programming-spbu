/**
 * @file vars.c
 * @brief Таблица переменных: массив записей, поиск перебором.
 */
#include "vars.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/** Одна запись таблицы. */
typedef struct {
    char name[VARS_NAME_MAX + 1]; /**< имя */
    double value;                 /**< значение */
} entry;

/** Устройство таблицы: растущий массив записей. */
struct vars {
    entry *items;   /**< массив записей */
    size_t count;   /**< занято */
    size_t cap;     /**< выделено */
};

/**
 * @brief Проверяет имя: длина 1..::VARS_NAME_MAX, буква или `_`, дальше буквы, цифры, `_`.
 * @param name имя
 * @return true, если имя годится
 */
static bool name_ok(const char *name)
{
    size_t len = strlen(name);
    if (len == 0 || len > VARS_NAME_MAX)
        return false;
    if (!isalpha((unsigned char)name[0]) && name[0] != '_')
        return false;
    for (size_t i = 1; i < len; i++)
        if (!isalnum((unsigned char)name[i]) && name[i] != '_')
            return false;
    return true;
}

/**
 * @brief Ищет запись по имени.
 * @param table таблица
 * @param name  имя
 * @return запись или NULL
 */
static entry *find(const vars *table, const char *name)
{
    for (size_t i = 0; i < table->count; i++)
        if (strcmp(table->items[i].name, name) == 0)
            return &table->items[i];
    return NULL;
}

vars *vars_new(void)
{
    return calloc(1, sizeof(vars));
}

void vars_free(vars *table)
{
    if (table == NULL)
        return;
    free(table->items);
    free(table);
}

vars_status vars_set(vars *table, const char *name, double value)
{
    if (!name_ok(name))
        return VARS_BAD_NAME;

    entry *e = find(table, name);
    if (e != NULL) {
        e->value = value;
        return VARS_OK;
    }
    if (table->count >= VARS_LIMIT)
        return VARS_FULL;
    if (table->count == table->cap) {
        size_t cap = table->cap ? table->cap * 2 : 8;
        entry *bigger = realloc(table->items, cap * sizeof(entry));
        if (bigger == NULL)
            return VARS_NOMEM;
        table->items = bigger;
        table->cap = cap;
    }
    e = &table->items[table->count++];
    strcpy(e->name, name);
    e->value = value;
    return VARS_OK;
}

bool vars_get(const vars *table, const char *name, double *value)
{
    const entry *e = find(table, name);
    if (e == NULL)
        return false;
    if (value != NULL)
        *value = e->value;
    return true;
}

size_t vars_count(const vars *table)
{
    return table->count;
}
