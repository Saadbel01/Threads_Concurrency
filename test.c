#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "codexion.h"

void* routine(void* arg)
{
    int a = 1;
    printf("Test from threads\n");
    printf( "ok");
    return NULL;
}

int main(int argc, char* argv[])
{
    pthread_t t1;
    pthread_create(&t1, NULL, &routine, NULL);
    pthread_join(t1, NULL);
    return 0;
}