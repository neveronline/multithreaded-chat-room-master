#include "csapp.h"

struct timeval timeout = {0, 1};

void write_time_name_event(char *log_buf, char *name, char *event);

int main(int argc, char **argv){
  int chatfd;
  FILE *fp;
  char log_buf[MAXBUF], name[MAXBUF];

  // if(argc < 3){
  //   fprintf(stderr, "./chat [UNIX_SOCKET_FD] [AUDIT_FILE_FD]\n" );
  //   exit(EXIT_FAILURE);
  // }
  chatfd = atoi(argv[1]);
  fp = fdopen(atoi(argv[2]), "a+");
  strcpy(name, argv[3]);
  char buf[MAXBUF];
  rio_t rio;
  fd_set read_set, ready_set;


  rio_readinitb(&rio, chatfd);

  FD_ZERO(&read_set);              /* Clear read set */ //line:conc:select:clearreadset
  FD_SET(STDIN_FILENO, &read_set); /* Add stdin to read set */ //line:conc:select:addstdin
  FD_SET(chatfd, &read_set);     /* Add clientfd to read set */ //line:conc:select:addclientfd

  while(1){
    ready_set = read_set;
    Select(chatfd+1, &ready_set, NULL, NULL, &timeout); //line:conc:select:select

    if (FD_ISSET(STDIN_FILENO, &ready_set)){
      memset(buf, '\0', strlen(buf));

      if (!Fgets(buf, MAXLINE, stdin))
      exit(0); /* EOF */

      if(!strcmp(buf, "/close\n")){
        flock(fileno(fp), LOCK_EX);
        write_time_name_event(log_buf, name, "CMD");
        strcat(log_buf, "/close, success, client\n");
        fputs(log_buf, fp);
        fflush(fp);
        flock(fileno(fp), LOCK_UN);

        break;
      }

      else if(!strncmp(buf, "/", 1)){
        buf[strlen(buf)-1] = '\0'; //remove \n

        flock(fileno(fp), LOCK_EX);
        write_time_name_event(log_buf, name, "CMD");
        strcat(log_buf, buf);
        strcat(log_buf, ", fail, client\n");
        fputs(log_buf, fp);
        fflush(fp);
        flock(fileno(fp), LOCK_UN);
      }


      else{
        Rio_writen(chatfd, buf, strlen(buf));
      }

      //printf("have written to client: %s\n", buf);
    }

    if (FD_ISSET(chatfd, &ready_set)){
      memset(buf, '\0', strlen(buf));
      if(Rio_readlineb(&rio, buf, MAXLINE) != 0){
        printf("%s", buf);
      }
    }

  }

  Close(chatfd);
  exit(0);
}

void write_time_name_event(char *log_buf, char *name, char *event){
  time_t rawtime;
  struct tm *info;

  time( &rawtime );

  info = localtime( &rawtime );

  memset(log_buf, '\0', strlen(log_buf));
  strftime(log_buf,80,"%x - %I:%M%p", info);
  //printf("Formatted date & time : |%s|\n", buffer );
  strcat(log_buf, ", ");
  strcat(log_buf, name);
  strcat(log_buf, ", ");
  strcat(log_buf, event);
  strcat(log_buf, ", ");
  printf("the log_buf is: %s\n", log_buf);
}
