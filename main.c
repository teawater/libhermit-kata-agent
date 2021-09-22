#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
    printf("Hello Kata libHermit WAMR! %d\n", argc);

    if (argc != 2) {
        printf("wasm file is not set\n");
        return -1;
    }

    return wamr(argv[1]);
}
