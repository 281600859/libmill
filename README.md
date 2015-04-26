MILL
====

This project is trying to introduce Go-style concurrency to C.

test.c:
```
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
        printf ("coroutine 'a' have finished first!");
    in(ch2, val):
        printf ("coroutine 'b' have finished first!");
    end
    }

    chclose(ch2);
    chclose(ch1);

    return 0;
}
```

To build the test above:
```
$ gcc -o test test.c mill.c
```

This is a proof of concept project that seems to work with x86-64, gcc
and Linux. I have no idea about different environments. Also, the project
is in very early stage of development and not suitable for actual usage.

Mailing List
------------

Discussion goes on at millc@freelists.org

Subscribe here:

http://www.freelists.org/list/millc

Archives:

http://www.freelists.org/archive/millc/
