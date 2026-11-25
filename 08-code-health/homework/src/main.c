/* main.c — маленькая программа, которая пользуется функциями.
 * Имя берётся из аргумента командной строки и сначала сокращается
 * до 10 букв (shorten_name), потом попадает в приветствие.
 */
#include "greeting.h"
#include "shorten.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    char short_name[SHORT_NAME_CAP];
    char buffer[64];
    const char *name = (argc > 1) ? argv[1] : "world";

    if (shorten_name(short_name, sizeof short_name, name) < 0) {
        fprintf(stderr, "не удалось сократить имя\n");
        return 1;
    }

    int rc = make_greeting(buffer, sizeof buffer, short_name);
    if (rc != 0) {
        fprintf(stderr, "не удалось собрать приветствие (код %d)\n", rc);
        return 1;
    }

    printf("%s\n", buffer);
    return 0;
}
