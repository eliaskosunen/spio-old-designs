#include <stdio.h>

int main() {
    char buf[10] = {0};
    fread(buf, 1, 10, stdin);
    fwrite(buf, 1, 10, stdout);
    return 0;
}
