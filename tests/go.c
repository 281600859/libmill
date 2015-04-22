
#include <stdio.h>
#include "../mill.c"

void worker(int count, const char *test) {
    int i;
    for(i = 0; i != count; ++i) {
        printf("%s\n", test);
        musleep(10000);
    }
}

int main() {
    go(worker(3, "a"));
    go(worker(1, "b"));
    go(worker(2, "c"));
    musleep(50000);
    return 0;
}

