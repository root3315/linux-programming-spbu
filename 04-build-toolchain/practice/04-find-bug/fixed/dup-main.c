/* dup-main.c — моя точка входа для проверки duplicate-a.c + duplicate-b.c.
 * В примере из Chapter 04 main нет, поэтому после исправления helper
 * линковщик всё равно ругается на undefined reference to `main'.
 */
void print_a(void);
void print_b(void);

int main(void)
{
    print_a();
    print_b();
    return 0;
}
