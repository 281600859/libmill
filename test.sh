./mill sleep.mill
gcc -o sleep sleep.c mill.c -luv -g -O0
./sleep
./mill tcpsocket.mill
gcc -o sleep tcpsocket.c mill.c -luv -g -O0
./tcpsocket

./mill hello.mill
gcc -o hello hello.c mill.c -luv -g -O0
./mill world.mill
gcc -o world world.c mill.c -luv -g -O0
