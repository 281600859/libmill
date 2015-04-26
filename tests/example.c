
#include <stdio.h>
#include "../mill.h"

void worker(int count, const char *text, chan ch) {
    int i;
    for(i = 0; i != count; ++i) {
        printf("%s\n", text);
        yield();
    }
    chs(ch, NULL);
    chclose(ch);
}

int main() {

    chan ch1 = chmake();
    go(worker(4, "a", ch1));
    chan ch2 = chmake();
    go(worker(2, "b", ch2));

    choose {
    in(ch1, val):
        printf("coroutine 'a' have finished first!");
    in(ch2, val):
        printf("coroutine 'b' have finished first!");
    end
    }

    chclose(ch2);
    chclose(ch1);

    return 0;
}

