#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

 void sfwrite(pthread_mutex_t *lock, FILE* stream, char *fmt, ...);
