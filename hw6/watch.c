#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <unistd.h>

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

int main( int argc, char **argv )
{
  int length, i;
  int fd;
  int wd;
  char buffer[BUF_LEN], c, buf[1024], file[1024];
  FILE *fp;

  memset(file, '\0', 1024);
  strcpy(file, argv[1]);

  fd = inotify_init();

  if ( fd < 0 ) {
    perror( "inotify_init" );
  }

  wd = inotify_add_watch( fd, ".", IN_MODIFY);

  while(1){

    length = read(fd, buffer, BUF_LEN);

    if ( length < 0 ) {
      perror( "read" );
    }

    i=0;
    while ( i < length ) {
      struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
      if ( event->len ) {
        if ( event->mask & IN_MODIFY ) {
          //get the last line of file and print it
          fp = fopen(file, "r");
          fseek(fp, -2, SEEK_END);

          c = fgetc(fp);
          while(c != '\n'){
            fseek(fp, -2, SEEK_CUR);
            c = fgetc(fp);
          }

          fseek(fp, 2, SEEK_CUR);

          memset(buf, '\0', 1024);
          fgets(buf, 1024, fp);
          printf("last line: %s", buf);
        }
      }
      i += EVENT_SIZE + event->len;
    }

  }

  (void)inotify_rm_watch(fd, wd);
  (void)close(fd);

  exit( 0 );
}
