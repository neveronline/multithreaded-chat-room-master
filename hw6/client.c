#include "csapp.h"
#include "sfwrite.h"

struct timeval timeout = {0, 1};

typedef struct chats_list{
  char to[MAXBUF], from[MAXBUF];
  int client_listen_fd;
  int is_chat_window_on;
  struct chats_list *next;
  struct chats_list *prev;
} chats;
pthread_mutex_t lock ;
void display_help();
chats* new_chat(int client_listen_fd, int is_chat_window_on, char *to, char *from);
void add_chat(chats* chat);
void remove_chat(int client_listen_fd);
void write_time_name_event(char *log_buf, char *name, char *event);

chats *head = NULL;

int main(int argc, char **argv)
{
  int clientfd, client_listen_fd = 0, port, c, new_user = 0, verbose = 0, is_login = 0;
  char *host, buf[MAXLINE], name[MAXBUF], log_buf[MAXBUF], port_str[MAXBUF],log_name[32];
  char to[MAXBUF], from[MAXBUF], msg[MAXBUF];
  rio_t rio, chat_rio;
  fd_set read_set, ready_set;
  FILE *fp;
  pthread_mutex_init(&lock,NULL);
  int chat = 0;


  // if (argc != 4) {
  //   fprintf(stderr, "usage: %s <name> <host> <port>\n", argv[0]);
  //   exit(0);
  // }

  memset(log_name, '\0', 32);
  while((c = getopt(argc, argv, "hc:va:")) != -1){
    switch(c){
      case 'h':
      display_help();
      return EXIT_SUCCESS;

      case 'c':
      new_user = 1;
      strcpy(name, optarg);
      //printf("%s\n", "creating new user...");
      break;

      case 'v':
      verbose = 1;
      break;

      case 'a':
      strcpy(log_name, optarg);

      default:
      //printf("%s\n", "Not Valid Option");
      break;
    }
  }

  if(argc > optind){
    if(new_user){
      host = argv[optind];
      port = atoi(argv[optind+1]);
      strcpy(port_str, argv[optind+1]);
    } else {
      strcpy(name, argv[optind]);
      host = argv[optind+1];
      port = atoi(argv[optind+2]);
      strcpy(port_str, argv[optind+2]);
    }
  }

  //open audit.log
  if(log_name[0] == '\0'){
    fp = fopen("audit.log", "a+");
  } else {
    fp = fopen(log_name, "a+");
  }


  clientfd = Open_clientfd(host, port);
  Rio_readinitb(&rio, clientfd);

  //send
  memset(buf, '\0', strlen(buf));
  strcpy(buf, "WOLFIE \r\n\r\n");
  Rio_writen(clientfd, buf, strlen(buf));


  write_time_name_event(log_buf, name, "LOGIN");
  strcat(log_buf, host);
  strcat(log_buf, ":");
  strcat(log_buf, port_str);
  strcat(log_buf, ", ");

  FD_ZERO(&read_set);              /* Clear read set */ //line:conc:select:clearreadset
  FD_SET(STDIN_FILENO, &read_set); /* Add stdin to read set */ //line:conc:select:addstdin
  FD_SET(clientfd, &read_set);     /* Add clientfd to read set */ //line:conc:select:addclientfd

  while (1) {
    ready_set = read_set;
    if(clientfd > client_listen_fd){
      Select(clientfd+1, &ready_set, NULL, NULL, &timeout); //line:conc:select:select
    } else {
      Select(client_listen_fd+1, &ready_set, NULL, NULL, &timeout); //line:conc:select:select
    }
    // fprintf(stderr, "Select triggered\n");
    if (FD_ISSET(STDIN_FILENO, &ready_set)) {
      memset(buf, '\0', strlen(buf));

      if (!Fgets(buf, MAXLINE, stdin))
      exit(0); /* EOF */

      if(!strcmp(buf, "/help\n")){
        display_help();
        flock(fileno(fp), LOCK_EX);
        write_time_name_event(log_buf, name, "CMD");
        strcat(log_buf, "/help, success, client\n");
        fputs(log_buf, fp);
        fflush(fp);
        flock(fileno(fp), LOCK_UN);
      }

      else if(!strcmp(buf, "/logout\n")){
        memset(buf, '\0', strlen(buf));
        strcpy(buf, "BYE \r\n\r\n");

        flock(fileno(fp), LOCK_EX);
        write_time_name_event(log_buf, name, "LOGOUT");
        strcat(log_buf, "intentional\n");
        fputs(log_buf, fp);
        fflush(fp);
        flock(fileno(fp), LOCK_UN);

        Rio_writen(clientfd, buf, strlen(buf));
      }

      else if(!strcmp(buf, "/time\n")){
        memset(buf, '\0', strlen(buf));
        strcpy(buf, "TIME \r\n\r\n");

        flock(fileno(fp), LOCK_EX);
        write_time_name_event(log_buf, name, "CMD");
        strcat(log_buf, "/time, success, client\n");
        fputs(log_buf, fp);
        fflush(fp);
        flock(fileno(fp), LOCK_UN);

        Rio_writen(clientfd, buf, strlen(buf));

      }

      else if(!strcmp(buf, "/listu\n")){
        memset(buf, '\0', strlen(buf));
        strcpy(buf, "LISTU \r\n\r\n");

        flock(fileno(fp), LOCK_EX);
        write_time_name_event(log_buf, name, "CMD");
        strcat(log_buf, "/listu, success, client\n");
        fputs(log_buf, fp);
        fflush(fp);
        flock(fileno(fp), LOCK_UN);

        Rio_writen(clientfd, buf, strlen(buf));
      }

      else if(!strncmp(buf, "/chat ", 6)){
        // eg. /chat Neal Hi Neal!
        //char to[MAXBUF], msg[MAXBUF];
        memset(to, '\0', strlen(to));
        memset(msg, '\0', strlen(msg));
        int count = 0, i = 6;
        while(buf[i] != ' '){
          i++;
          count++;
        }
        strncpy(to, &buf[6], count);
        strcpy(msg, &buf[i+1]);
        msg[strlen(msg)-1] = '\0'; //remove \n

        memset(buf, '\0', strlen(buf));
        strcpy(buf, "MSG ");
        strcat(buf, to);
        strcat(buf, " ");
        strcat(buf, name);
        strcat(buf, " ");
        strcat(buf, msg);
        strcat(buf, " \r\n\r\n");
        //printf("the msg is: %s", buf);

        flock(fileno(fp), LOCK_EX);
        write_time_name_event(log_buf, name, "CMD");
        strcat(log_buf, "/chat, ");
        chat = 1;


        Rio_writen(clientfd, buf, strlen(buf));

      }

      else if(!strcmp(buf, "/audit\n")){
        flock(fileno(fp), LOCK_EX);
        write_time_name_event(log_buf, name, "CMD");
        strcat(log_buf, "/audit, success, client\n");
        fputs(log_buf, fp);
        fflush(fp);
        flock(fileno(fp), LOCK_UN);

        if(log_name[0] == '\0'){
          fp = fopen("audit.log", "a+");
        } else {
          fp = fopen(log_name, "a+");
        }
        memset(buf, '\0', strlen(buf));
        flock(fileno(fp), LOCK_EX);
        while(fgets(buf, sizeof(buf), fp) != NULL){
          // printf("%s", buf);
          sfwrite(&lock,stdout,"%s", buf);
          memset(buf, '\0', strlen(buf));
        }
        flock(fileno(fp), LOCK_UN);


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

      else {
        Rio_writen(clientfd, buf, strlen(buf));
      }

    }

    while (FD_ISSET(clientfd, &ready_set)) { //line:conc:select:clientfdready
      //printf("%s\n", "received from server");
      memset(buf, '\0', strlen(buf));
      if(Rio_readlineb(&rio, buf, MAXLINE) != 0){

        if(!strncmp(buf, "EIFLOW \r\n\r\n", strlen(buf))){
          if(verbose){
            sfwrite(&lock,stdout,"%s%s%s", VERBOSE, buf, DEFAULT);
            // printf("%s%s%s", VERBOSE, buf, DEFAULT);
          }
          memset(buf, '\0', strlen(buf));
          if(new_user){
            strcpy(buf, "IAMNEW ");
          } else {
            strcpy(buf, "IAM ");
          }
          strcat(buf, name);
          strcat(buf, " \r\n\r\n");
          Rio_writen(clientfd, buf, strlen(buf));
        }

        else if(!strncmp(buf, "MOTD ", 5)){
          if(verbose){
            sfwrite(&lock,stdout,"%s%s%s", VERBOSE, buf, DEFAULT);
            // printf("%s%s%s", VERBOSE, buf, DEFAULT);
          }
          Rio_writen(STDOUT_FILENO, &buf[5], strlen(buf)-5);
          is_login = 1;

          strcat(log_buf, "success, ");
          strcat(log_buf, &buf[5]);
          //strcat(log_buf, "\n");

          fputs(log_buf, fp);
          fflush(fp);
          flock(fileno(fp), LOCK_UN);

          break;
        }

        else if(!strncmp(buf, "ERR ", 4)){
          if(verbose){
            sfwrite(&lock,stdout,"%s%s%s", VERBOSE, buf, DEFAULT);
            // printf("%s%s%s", ERRORS, buf, DEFAULT);
          }
          if(is_login){
            Rio_writen(STDOUT_FILENO, &buf[7], strlen(buf)-7);

            if(chat){
              strcat(log_buf, "fail, client\n");
              fputs(log_buf, fp);
              fflush(fp);
              flock(fileno(fp), LOCK_UN);
              chat = 0;
            }

            break;
          } else {
            strcat(log_buf, "fail, ");
            strcat(log_buf, buf);
            //strcat(log_buf, "\n");
            flock(fileno(fp), LOCK_EX);
            fputs(log_buf, fp);
            fflush(fp);
            flock(fileno(fp), LOCK_UN);

            Rio_writen(STDOUT_FILENO, &buf[7], strlen(buf)-7);
          }
        }

        else if(!strncmp(buf, "BYE \r\n\r\n", strlen(buf))){
          if(verbose){
            sfwrite(&lock,stdout,"%s%s%s", VERBOSE, buf, DEFAULT);
            // printf("%s%s%s", VERBOSE, buf, DEFAULT);
          }
          goto END;
        }

        else if(!strncmp(buf, "UTSIL ", 6)){
          // memset(buf, '\0', strlen(buf));
          // printf("%s\n",rio.rio_buf );
          if(verbose){
            sfwrite(&lock,stdout,"%s%s%s", VERBOSE, rio.rio_buf, DEFAULT);
            // printf("%s%s%s", VERBOSE, buf, DEFAULT);
          }
          Rio_writen(STDOUT_FILENO, &(rio.rio_buf)[6], strlen(rio.rio_buf)-6);
          break;
        }

        else if(!strncmp(buf, "EMIT ", 5)){
          if(verbose){
            sfwrite(&lock,stdout,"%s%s%s", VERBOSE, buf, DEFAULT);
            // printf("%s%s%s", VERBOSE, buf, DEFAULT);
          }
          int hour, min, sec;
          int time_duration = atoi(&buf[5]);
          hour = time_duration / 3600;
          min = (time_duration % 3600) / 60;
          sec = (time_duration % 3600) % 60;
          sfwrite(&lock,stdout,"connected for %d hour(s), %d minute(s), and %d second(s)\n", hour, min, sec);

          // printf("connected for %d hour(s), %d minute(s), and %d second(s)\n", hour, min, sec);

          break;
        }

        else if(!strncmp(buf, "HINEW ", 6)){
          if(verbose){
            // printf("%s%s%s", VERBOSE, buf, DEFAULT);
            sfwrite(&lock,stdout,"%s%s%s", VERBOSE, buf, DEFAULT);
          }
          char password[MAXBUF];
          sfwrite(&lock,stdout,"Please set a password: ");
          // printf("Please set a password: ");

          memset(password, '\0', strlen(password));
          if (!Fgets(password, MAXLINE, stdin))
          exit(0); /* EOF */
          password[strlen(password)-1] = '\0'; //remove \n
          memset(buf, '\0', strlen(buf));
          strcpy(buf, "NEWPASS ");
          strcat(buf, password);
          strcat(buf, " \r\n\r\n");
          Rio_writen(clientfd, buf, strlen(buf));
        }

        else if(!strncmp(buf, "AUTH ", 5)){
          if(verbose){
            // printf("%s%s%s", VERBOSE, buf, DEFAULT);
            sfwrite(&lock,stdout,"%s%s%s", VERBOSE, buf, DEFAULT);
          }
          // char password[MAXBUF];
          // printf("Please enter your password: ");
          char *password;
          password = getpass("Please enter your password: ");
          // memset(password, '\0', strlen(password));
          // if (!Fgets(password, MAXLINE, stdin))
          // exit(0); /* EOF */
          // password[strlen(password)-1] = '\0'; //remove \n
          memset(buf, '\0', strlen(buf));
          strcpy(buf, "PASS ");
          strcat(buf, password);
          strcat(buf, " \r\n\r\n");
          Rio_writen(clientfd, buf, strlen(buf));
        }



        else if(!strncmp(buf, "MSG ", 4)){
          if(verbose){
            // printf("%s%s%s", VERBOSE, buf, DEFAULT);
            sfwrite(&lock,stdout,"%s%s%s", VERBOSE, buf, DEFAULT);
          }
          //open xterm
          sfwrite(&lock,stdout,"got msg back form server: %s", buf);
          // printf("got msg back form server: %s", buf);

          if(chat){
            strcat(log_buf, "success, client\n");
            fputs(log_buf, fp);
            fflush(fp);
            flock(fileno(fp), LOCK_UN);
            chat = 0;
          }

          //parse the "from" and "to"
          int i = 4, count = 0, is_connected = 0;


          memset(to, '\0', strlen(to));
          memset(from, '\0', strlen(from));
          memset(msg, '\0', strlen(msg));

          while(buf[i] != ' '){
            i++;
            count++;
          }

          strncpy(to, &buf[4], count);
          count = 0;
          i++;

          while(buf[i] != ' '){
            i++;
            count++;
          }
          strncpy(from, &buf[i-count], count);
          count = 0;
          i++;

          while(buf[i] != ' '){
            i++;
            count++;
          }
          strncpy(msg, &buf[i-count], count);
          strcat(msg, "\n");

          chats *cursor = head;
          while(cursor != NULL){
            if((!strcmp(cursor->to, to) && !strcmp(cursor->from, from)) || (!strcmp(cursor->to, from) && !strcmp(cursor->from, to))){
              is_connected = 1;
              break;
            }
            cursor = cursor->next;
          }

          int socket_pair[2];
          if(!is_connected){
            if(socketpair(AF_UNIX,SOCK_STREAM,0,socket_pair) == -1){
              // fprintf(stderr, "socket pair failed\n" );
              sfwrite(&lock,stderr, "socket pair failed\n");

            }

            client_listen_fd = socket_pair[0];
            rio_readinitb(&chat_rio, client_listen_fd);
            FD_SET (client_listen_fd, &read_set);
            chats *chat = new_chat(client_listen_fd, 0, to, from);
            add_chat(chat);
            cursor = chat;

            is_connected = 0; //reset after use it.
          }

          if(cursor->is_chat_window_on == 0){
            pid_t pid;
            if ((pid = fork()) == 0){

              char fd_string[10], fp_string[10];
              sprintf(fd_string, "%d",socket_pair[1]);
              sprintf(fp_string, "%d", fileno(fp));

              close(socket_pair[0]);
              char* xterm_arg[20];
              xterm_arg[0] = "xterm";
              xterm_arg[1] = "-geometry";
              xterm_arg[2] = "45*35+1";
              xterm_arg[3] = "-T";

              char temp[100];
              strcpy(temp,"chat with ");
              char tempname[100];
              if(!strcmp(to,name)){
                strcpy(tempname,from);
                strcat(temp,tempname);
              }
              else{
                strcpy(tempname,to);
                strcat(temp,tempname);
              }


              xterm_arg[4] = temp;
              xterm_arg[5] = "-e";
              xterm_arg[6] = "./chat";
              xterm_arg[7] = fd_string;
              xterm_arg[8] = fp_string;
              xterm_arg[9] = name;
              execvp(xterm_arg[0],xterm_arg);
            }
            close(socket_pair[1]);
            cursor->is_chat_window_on = 1;

          }

          //send the msg to chat window
          //use name to check the msg sender or reveiver
          memset(buf, '\0', strlen(buf));
          if(!strcmp(name, to)){
            strcpy(buf, "> ");
          }

          else if(!strcmp(name, from)){
            strcpy(buf, "< ");
          }
          strcat(buf, msg);
          Rio_writen(client_listen_fd, buf, strlen(buf));

          break;
        }



      }


    }

    //check chatfd set
    chats *cursor = head;
    while(cursor != NULL){
      if(FD_ISSET(cursor->client_listen_fd, &ready_set)){
        memset(buf, '\0', strlen(buf));
        if(Rio_readlineb(&chat_rio, buf, MAXLINE) != 0) {

          //printf("get msg from chat: %s", buf);
          buf[strlen(buf)-1] = '\0'; //remove \n

          memset(msg, '\0', strlen(msg));
          memset(from, '\0', strlen(from));
          memset(to, '\0', strlen(to));

          strcpy(msg, buf);
          strcpy(from, name);
          if(!strcmp(from, cursor->from)){
            strcpy(to, cursor->to);
          } else {
            strcpy(to, cursor->from);
          }

          memset(buf, '\0', strlen(buf));
          strcpy(buf, "MSG ");
          strcat(buf, to);
          strcat(buf, " ");
          strcat(buf, from);
          strcat(buf, " ");
          strcat(buf, msg);
          strcat(buf, " \r\n\r\n");

          Rio_writen(clientfd, buf, strlen(buf));

          flock(fileno(fp), LOCK_EX);
          write_time_name_event(log_buf, name, "MSG");

          strcat(log_buf, "to, ");
          strcat(log_buf, to);
          strcat(log_buf, ", ");
          strcat(log_buf, msg);
          strcat(log_buf, "\n");

          fputs(log_buf, fp);
          fflush(fp);
          flock(fileno(fp), LOCK_UN);


        }

      }
      cursor = cursor->next;
    }
  }

  END:
  Close(clientfd);
  fclose(fp);
  exit(0);
}

void display_help(){
  sfwrite(&lock,stdout,"/help\t%s\n", "List all the commands which the client accepts.");
  sfwrite(&lock,stdout,"/logout\t%s\n", "Client will disconnect with the server.");
  sfwrite(&lock,stdout,"/listu\t%s\n", "Connected users list will be displayed.");

  // printf("/help\t%s\n", "List all the commands which the client accepts.");
  // printf("/logout\t%s\n", "Client will disconnect with the server.");
  // printf("/listu\t%s\n", "Connected users list will be displayed.");
}

chats* new_chat(int client_listen_fd, int is_chat_window_on, char *to, char *from) {
  chats* chat = (chats *)malloc(sizeof(chats));
  chat->client_listen_fd = client_listen_fd;
  chat->is_chat_window_on = is_chat_window_on;
  strcpy(chat->to, to);
  strcpy(chat->from, from);
  chat->prev = NULL;
  chat->next = NULL;
  return chat;
}

void add_chat(chats* chat) {
  chats *cursor = head;
  if(head == NULL){
    head = chat;
    return;
  }
  while (cursor->next != NULL) {
    cursor = cursor->next;
  }
  cursor->next = chat;
  chat->prev = cursor;
  chat->next = NULL;
}

void remove_chat(int client_listen_fd) {
  chats *cursor = head;
  while(cursor != NULL){
    if(cursor->client_listen_fd == client_listen_fd){
      if(cursor->next == NULL){
        cursor->prev->next = NULL;
      }
      else if(cursor->prev == NULL){
        cursor->next->prev = NULL;
      }
      else {
        cursor->prev->next = cursor->next;
        cursor->next->prev = cursor->prev;
      }
      free(cursor);
    }
    cursor = cursor->next;
  }
}

void write_time_name_event(char *log_buf, char *name, char *event){
  time_t rawtime;
  struct tm *info;

  time( &rawtime );

  info = localtime( &rawtime );

  memset(log_buf, '\0', strlen(log_buf));
  strftime(log_buf,80,"%x-%I:%M%p", info);
  //printf("Formatted date & time : |%s|\n", buffer );
  strcat(log_buf, ", ");
  strcat(log_buf, name);
  strcat(log_buf, ", ");
  strcat(log_buf, event);
  strcat(log_buf, ", ");
}
/* $end select */
