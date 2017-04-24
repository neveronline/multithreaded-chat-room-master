#include "sfwrite.h"

 void sfwrite(pthread_mutex_t *lock, FILE* stream, char *fmt, ...)
 {
   pthread_mutex_lock(lock);
   va_list args;
   va_start(args,fmt);
   vfprintf(stream,fmt,args);
   va_end(args);

   pthread_mutex_unlock(lock);
 }
