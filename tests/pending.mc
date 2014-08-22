
#include <assert.h>
#include <stddef.h>

#include "../stdmill.mh"

coroutine fx1 (out int *idout, int id, int milliseconds)
{
    go msleep (milliseconds);
    select {
    case msleep:
    }
    *idout = id;
}

coroutine fx2 = fx1;
coroutine fx3 = fx1;

coroutine test ()
{
    int id;
    endvars;

    go fx1 (&id, 3, 150);
    go fx1 (&id, 1, 100);
    go fx2 (&id, 2, 200);
    select {
    case fx2:
        assert (id == 2);
    }
    select {
    case fx1:
        assert (id == 1);
    }
    select {
    case fx1:
        assert (id == 3);
    }

    go fx1 (&id, 1, 50);
    go fx2 (&id, 2, 100);
    go fx3 (&id, 3, 150);
    select {
    case fx3:
        assert (id == 3);
    }
    select {
    case fx2:
        assert (id == 2);
    }
    select {
    case fx1:
        assert (id == 1);
    }
}

int main ()
{
    test();
}

