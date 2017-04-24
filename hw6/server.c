#include "csapp.h"
#include "sfwrite.h"
#include <sqlite3.h>
#include <openssl/sha.h>
#include <openssl/rand.h>

typedef struct { /* represents a pool of connected descriptors */ //line:conc:echoservers:beginpool
  int maxfd;        /* largest descriptor in read_set */
  fd_set read_set;  /* set of all active descriptors */
  fd_set ready_set; /* subset of descriptors ready for reading  */
  int nready;       /* number of ready descriptors from select */
  int maxi;         /* highwater index into client array */
  int clientfd[FD_SETSIZE];    /* set of active descriptors */
  rio_t clientrio[FD_SETSIZE]; /* set of active read buffers */
} pool; //line:conc:echoservers:endpool
struct timeval timeout = {0, 1};

typedef struct clients_list{
  char name[MAXBUF];
  int connfd;
  time_t login_time;
  //char password[MAXBUF];
  struct clients_list *next;
} clients;

typedef struct accounts_list{
  char name[MAXBUF];
  char password[MAXBUF];
  char salt[100];
  struct accounts_list *next;
} accounts;

typedef struct login_queue{
  int *fd;
  struct login_queue *next;
}login_queue;

void init_pool(int listenfd, pool *p);
void add_client(int connfd, pool *p);
void check_clients(pool *p);
void *login_thread();
void *echo_thread(void *vargp);
void display_cmd();
void display_user();
void display_accts_info();
int check_name(char *name); //check whether this user has logged in
int check_username(char *name);
int check_password(char *password); //validate password
int check_user_password(char *password); //match it with the correct one in database
clients* new_client(char *name, int connfd);
clients* find_client(int connfd);
void add(clients *new_client);
void delete(clients *client);
int callback(void *NotUsed, int argc, char **argv, char **azColName);
accounts* new_account(char *name, char *password, char* salt);
void add_account(accounts *new_account);
int find_clientfd_by_name(char* name);
void ctrlC_handler();
char* get_salt_by_name (char *name);
void delete_from_queue (login_queue* request);
void add_to_queue(login_queue* request);
login_queue* new_login(int* fdt);
login_queue* handle_request();


int byte_cnt = 0; /* counts total bytes received by server */
int echo_flag = 0;
int verbose = 0;
static pool fd_pool;
char MOTD[MAXBUF];
clients *head = NULL;
accounts *first = NULL;
login_queue *top = NULL;
sqlite3 *db;
int listenfd;
pthread_mutex_t lock ;
pthread_mutex_t client_lock;
pthread_mutex_t account_lock;
pthread_mutex_t Q_lock;
sem_t items_sem;


int main(int argc, char **argv) {
  int *connfdp, port, c, num_of_thread;
  char num_of_thread_str[MAXBUF];
  socklen_t clientlen=sizeof(struct sockaddr_in);
  struct sockaddr_in clientaddr;
  pthread_t tid;
  rio_t rio;
  int rc;
  char *ErrMsg = 0;
  char *sql_create;
  char *sql_select;
  pthread_mutex_init(&lock,NULL);
  pthread_mutex_init(&Q_lock,NULL);
  pthread_mutex_init(&client_lock,NULL);
  pthread_mutex_init(&account_lock,NULL);
  Sem_init(&items_sem,0,0);


  // if (argc != 3) {
  //   fprintf(stderr, "usage: %s <port> <MOTD>\n", argv[0]);
  //   exit(0);
  // }
  while((c = getopt(argc, argv, "hvt:")) != -1){
    switch(c){
      case 'h':
      display_cmd();
      return EXIT_SUCCESS;

      case 'v':
      verbose = 1;
      break;

      case 't':
      strcpy(num_of_thread_str, optarg);
      num_of_thread = atoi(num_of_thread_str);
      break;

      default:
      // printf("%s\n", "Not Valid Option");
      sfwrite(&lock,stdout,"%s\n", "Not Valid Option");

      break;
    }
  }
  port = atoi(argv[optind]);
  strcpy(MOTD, argv[optind+1]);

  listenfd = Open_listenfd(port);
  init_pool(listenfd, &fd_pool);

  Rio_readinitb(&rio, STDIN_FILENO);
  add_client(STDIN_FILENO, &fd_pool);

  rc = sqlite3_open("accounts.db", &db);

  /* Create SQL statement */


  sql_create = "CREATE TABLE IF NOT EXISTS USER("  \
  "NAME      CHAR(50)  PRIMARY KEY NOT NULL," \
  "PASSWORD  CHAR(50)  NOT NULL," \
  "SALT      CHAR(50)  NOT NULL);";

  rc = sqlite3_exec(db, sql_create, 0, 0, &ErrMsg);
  if( rc != SQLITE_OK ){
    sfwrite(&lock,stderr,"SQL error: %s\n", ErrMsg);
    // fprintf(stderr, "SQL error: %s\n", ErrMsg);
    sqlite3_free(ErrMsg);
  }else{
    sfwrite(&lock,stdout,"Table created successfully\n");
    // fprintf(stdout, "Table created successfully\n");
  }

  sql_select = "SELECT * FROM USER";
  rc = sqlite3_exec(db, sql_select, callback, 0, &ErrMsg);
  if( rc != SQLITE_OK ){
    sfwrite(&lock,stderr,"SQL error: %s\n", ErrMsg);
    // fprintf(stderr, "SQL error: %s\n", ErrMsg);
    sqlite3_free(ErrMsg);
  }else{
    sfwrite(&lock,stdout,"Operation done successfully\n");
    // fprintf(stdout, "Operation done successfully\n");
  }


  int index;
  for (index = 0; index < num_of_thread; index++){
    Pthread_create(&tid, NULL, login_thread, NULL);
  }
  while (1) {
    connfdp = Malloc(sizeof(int)); //line:conc:echoservert:beginmalloc
    *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen); //line:conc:echoservert:endmalloc
    pthread_mutex_lock(&Q_lock);
    // insert to queue
    login_queue* request = new_login(connfdp);
    add_to_queue(request);
    pthread_mutex_unlock(&Q_lock);
    V(&items_sem);
  }
}

/* thread routine */
void *login_thread() {



  char buf[MAXBUF];
  char name[MAXBUF];
  char password[MAXBUF];
  char finalpas[MAXBUF];
  char saltbuf[100];
  signal(SIGINT,&ctrlC_handler);

  pthread_t tid;
  Pthread_detach(pthread_self());

  while(1){


    P(&items_sem);


    pthread_mutex_lock(&Q_lock);
    login_queue* cursor = handle_request();
    int connfd = *((int *)cursor->fd);

    delete_from_queue(cursor);

    rio_t rio;
    Rio_readinitb(&rio, connfd);
    pthread_mutex_unlock(&Q_lock);



    //to do
    //deal with login queries
    while(1){

      memset(buf, '\0', strlen(buf));

      if(Rio_readlineb(&rio, buf, MAXLINE) != 0){

        if(verbose){
          sfwrite(&lock,stdout,"%s%s%s", VERBOSE, buf, DEFAULT);
          // printf("%s%s%s", VERBOSE, buf, DEFAULT);
        }

        // pthread_mutex_lock(&client_lock);
        if(!strncmp(buf, "WOLFIE \r\n\r\n", strlen(buf))){
          memset(buf, '\0', strlen(buf));
          strcpy(buf, "EIFLOW \r\n\r\n");
          Rio_writen(connfd, buf, strlen(buf));
        }

        else if(!strncmp(buf, "IAM ", 4)){
          strcpy(name, &buf[4]);
          name[strlen(name)-3] = '\0';

          //check username
          if(check_username(name)){ //did not find this username in account list
            //err 01
            memset(buf, '\0', strlen(buf));
            strcpy(buf, "ERR 01 USER NOT AVAILABLE \r\n\r\n");
            Rio_writen(connfd, buf, strlen(buf));
            memset(buf, '\0', strlen(buf));
            strcpy(buf, "BYE \r\n\r\n");
            Rio_writen(connfd, buf, strlen(buf));
            break;
          } else {
            memset(buf, '\0', strlen(buf));
            strcpy(buf, "AUTH ");
            strcat(buf, name);
            strcat(buf, " \r\n\r\n");
            Rio_writen(connfd, buf, strlen(buf));
          }



        }

        else if(!strncmp(buf, "IAMNEW ", 7)){
          int username_is_valid;
          strcpy(name, &buf[7]);
          name[strlen(name)-3] = '\0';

          //check  username in database
          username_is_valid = check_username(name);

          if(username_is_valid){
            sfwrite(&lock,stdout,"Username is valid \n");
            // printf("%s\n", "Username is valid");
            memset(buf, '\0', strlen(buf));
            strcpy(buf, "HINEW ");
            strcat(buf, name);
            strcat(buf, " \r\n\r\n");

            Rio_writen(connfd, buf, strlen(buf));

          } else {
            //send err back
            memset(buf, '\0', strlen(buf));
            strcpy(buf, "ERR 00 USER NAME TAKEN ");
            strcat(buf, name);
            strcat(buf, " \r\n\r\n");
            Rio_writen(connfd, buf, strlen(buf));
            memset(buf, '\0', strlen(buf));
            strcpy(buf, "BYE \r\n\r\n");
            Rio_writen(connfd, buf, strlen(buf));
            break;
          }
        }

        else if(!strncmp(buf, "PASS ", 5)){
          strcpy(password, &buf[5]);
          password[strlen(password)-3] = '\0';
          char* salt;
          if((salt = get_salt_by_name(name))!= NULL){
            strcat (password,salt);
            memset(finalpas, '\0', strlen(finalpas));
            SHA256((const unsigned char *)password, strlen(password), (unsigned char *)finalpas);
          }
          else{
            sfwrite(&lock,stderr,"the user name is not registered yet\n");
            // fprintf(stderr, "%s\n", "the user name is not registered yet");
          }

          //check whether this acount has logged in
          if(check_name(name)){ //this account is not logged in yet

          }

          else {
            memset(buf, '\0', strlen(buf));
            strcpy(buf, "ERR 00 USER NAME TAKEN \r\n\r\n");
            Rio_writen(connfd, buf, strlen(buf));
            memset(buf, '\0', strlen(buf));
            strcpy(buf, "BYE \r\n\r\n");
            Rio_writen(connfd, buf, strlen(buf));
            break;
          }

          //check pass in database
          if(check_user_password(finalpas)){
            //fprintf(stdout, "This password is valid.\n");
            memset(buf, '\0', strlen(buf));
            strcpy(buf, "SSAP \r\n\r\n");
            Rio_writen(connfd, buf, strlen(buf));

            memset(buf, '\0', strlen(buf));
            strcpy(buf, "HI ");
            strcat(buf, name);
            strcat(buf, " \r\n\r\n");
            Rio_writen(connfd, buf, strlen(buf));

            add_client(connfd, &fd_pool);
            clients *client = new_client(name, connfd);
            add(client); //add to list

            memset(buf, '\0', strlen(buf));
            strcpy(buf, "MOTD ");
            strcat(buf, MOTD);
            strcat(buf, " \r\n\r\n");
            Rio_writen(connfd, buf, strlen(buf));
            break;
          }
          else{
            memset(buf, '\0', strlen(buf));
            strcpy(buf, "ERR 02 BAD PASSWORD \r\n\r\n");
            Rio_writen(connfd, buf, strlen(buf));

            memset(buf, '\0', strlen(buf));
            strcpy(buf, "BYE \r\n\r\n");
            Rio_writen(connfd, buf, strlen(buf));
            break;
          }

        }

        else if(!strncmp(buf, "NEWPASS ", 8)){
          strcpy(password, &buf[8]);
          password[strlen(password)-3] = '\0';

          //check password
          if(check_password(password)){
            //add to database
            int rc;
            char sql_insert[MAXBUF];
            char *ErrMsg = 0;

            memset(saltbuf, '\0', strlen(saltbuf));
            RAND_bytes((unsigned char *)saltbuf, 0);
            char temp[100];
            strcpy(temp,saltbuf);
            strcat(password, temp);
            memset(finalpas, '\0', strlen(finalpas));
            SHA256((const unsigned char *)password, strlen(password), (unsigned char *)finalpas);

            strcpy(sql_insert, "INSERT INTO USER (NAME, PASSWORD, SALT) VALUES (");
            strcat(sql_insert, "\'");
            strcat(sql_insert, name);
            strcat(sql_insert, "\'");
            strcat(sql_insert, ", ");
            strcat(sql_insert, "\'");
            strcat(sql_insert, finalpas);
            strcat(sql_insert, "\'");
            strcat(sql_insert, ", ");
            strcat(sql_insert, "\'");
            strcat(sql_insert, saltbuf);
            strcat(sql_insert, "\'");
            strcat(sql_insert, ");");

            rc = sqlite3_exec(db, sql_insert, 0, 0, &ErrMsg);
            if( rc != SQLITE_OK ){
              sfwrite(&lock,stderr,"SQL error: %s\n", ErrMsg);
              // fprintf(stderr, "SQL error: %s\n", ErrMsg);
              sqlite3_free(ErrMsg);
            }else{
              sfwrite(&lock,stdout,"Records created successfully\n");
              // fprintf(stdout, "Records created successfully\n");
            }

            accounts *account = new_account(name, password,saltbuf);
            add_account(account);

            //send validation msg back to client
            memset(buf, '\0', strlen(buf));
            strcpy(buf, "SSAPWEN \r\n\r\n");
            Rio_writen(connfd, buf, strlen(buf));

            memset(buf, '\0', strlen(buf));
            strcpy(buf, "HI ");
            strcat(buf, name);
            strcat(buf, " \r\n\r\n");
            Rio_writen(connfd, buf, strlen(buf));


            add_client(connfd, &fd_pool);
            clients *client = new_client(name, connfd);
            add(client); //add to list

            memset(buf, '\0', strlen(buf));
            strcpy(buf, "MOTD ");
            strcat(buf, MOTD);
            strcat(buf, " \r\n\r\n");
            Rio_writen(connfd, buf, strlen(buf));

            break;
          }
          else {
            memset(buf, '\0', strlen(buf));
            strcpy(buf, "ERR 02 BAD PASSWORD \r\n\r\n");
            Rio_writen(connfd, buf, strlen(buf));
            memset(buf, '\0', strlen(buf));
            strcpy(buf, "BYE \r\n\r\n");
            Rio_writen(connfd, buf, strlen(buf));
            break;
          }
        }
        // pthread_mutex_unlock(&client_lock);
      }

    }// inner while end tag





    //check echo_flag
    if(echo_flag == 0){
      Pthread_create(&tid, NULL, echo_thread, NULL);
      echo_flag = 1;

    }
  }
  //Close(connfd);
  return NULL;
}

int callback(void *NotUsed, int argc, char **argv, char **azColName){
  if(argv[0] != NULL || argv[1] != NULL){
    accounts *account = new_account(argv[0], argv[1], argv[2]);
    add_account(account);
  }

  return 0;
}

void *echo_thread(void *vargp){
  Pthread_detach(pthread_self());

  while(1){
    /* Wait for connected descriptor(s) to become ready */

    fd_pool.ready_set = fd_pool.read_set;
    fd_pool.nready = Select(fd_pool.maxfd+1, &fd_pool.ready_set, NULL, NULL, &timeout);

    /* Echo a text line from each ready connected descriptor */
    //printf("%s\n", "before check clients");

    check_clients(&fd_pool); //line:conc:echoservers:checkclients
    //printf("%s\n", "after check clients");


  }
  return NULL;
}

/* $begin init_pool */
void init_pool(int listenfd_p, pool *p)
{
  /* Initially, there are no connected descriptors */
  int i;
  p->maxi = -1;                   //line:conc:echoservers:beginempty
  for (i=0; i< FD_SETSIZE; i++)
  p->clientfd[i] = -1;        //line:conc:echoservers:endempty

  /* Initially, listenfd is only member of select read set */
  p->maxfd = listenfd_p;            //line:conc:echoservers:begininit
  FD_ZERO(&p->read_set);
  FD_SET(listenfd_p, &p->read_set); //line:conc:echoservers:endinit
}
/* $end init_pool */

/* $begin add_client */
void add_client(int connfd, pool *p)
{

  int i;
  p->nready--;
  for (i = 0; i < FD_SETSIZE; i++)  /* Find an available slot */
  if (p->clientfd[i] < 0) {
    /* Add connected descriptor to the pool */
    p->clientfd[i] = connfd;                 //line:conc:echoservers:beginaddclient
    Rio_readinitb(&p->clientrio[i], connfd); //line:conc:echoservers:endaddclient

    /* Add the descriptor to descriptor set */
    FD_SET(connfd, &p->read_set); //line:conc:echoservers:addconnfd

    /* Update max descriptor and pool highwater mark */
    if (connfd > p->maxfd) //line:conc:echoservers:beginmaxfd
    p->maxfd = connfd; //line:conc:echoservers:endmaxfd
    if (i > p->maxi)       //line:conc:echoservers:beginmaxi
    p->maxi = i;       //line:conc:echoservers:endmaxi
    break;
  }
  if (i == FD_SETSIZE) /* Couldn't find an empty slot */
  app_error("add_client error: Too many clients");


}
/* $end add_client */

/* $begin check_clients */
void check_clients(pool *p)
{

  int i, connfd, n;
  char buf[MAXLINE];
  rio_t rio;

  for (i = 0; (i <= p->maxi) && (p->nready > 0); i++) {
    connfd = p->clientfd[i];
    rio = p->clientrio[i];
    /* If the descriptor is ready, echo a text line from it */
    if ((connfd >= 0) && (FD_ISSET(connfd, &p->ready_set))) {

      p->nready--;
      memset(buf, '\0', strlen(buf));
      if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        byte_cnt += n; //line:conc:echoservers:beginecho
        sfwrite(&lock,stdout,"Server received %d (%d total) bytes on fd %d\n",
        n, byte_cnt, connfd);

        // printf("Server received %d (%d total) bytes on fd %d\n",
        // n, byte_cnt, connfd);

        if(!strcmp(buf, "/users\n")){
          display_user();
        }

        if(!strcmp(buf, "/accts\n")){
          display_accts_info();

        }

        else if(!strcmp(buf, "/help\n")){
          display_cmd();
        }

        else if(!strcmp(buf, "/shutdown\n")){
          for (i = 0; (i <= p->maxi); i++) {
            connfd = p->clientfd[i];
            memset(buf, '\0', strlen(buf));
            strcpy(buf, "BYE \r\n\r\n");
            Rio_writen(connfd, buf, strlen(buf));
            Close(connfd); //line:conc:echoservers:closeconnfd
            FD_CLR(connfd, &p->read_set); //line:conc:echoservers:beginremove
            p->clientfd[i] = -1;          //line:conc:echoservers:endremove
          }
          sqlite3_close(db);
          exit(0);
        }

        else if(!strncmp(buf, "LISTU", 5)){
          if(verbose){
            sfwrite(&lock,stdout,"%s%s%s", VERBOSE, buf, DEFAULT);
            // printf("%s%s%s", VERBOSE, buf, DEFAULT);
          }
          memset(buf, '\0', strlen(buf));
          pthread_mutex_lock(&client_lock);
          clients *cursor = head;
          strcpy(buf, "UTSIL ");
          while(cursor != NULL){
            strcat(buf, cursor->name);
            strcat(buf, "\r\n");
            if(cursor-> next != NULL)
            {
              strcat(buf," ");
            }
            cursor = cursor->next;
          }
          pthread_mutex_unlock(&client_lock);
          strcat(buf, "\r\n");
          Rio_writen(connfd, buf, strlen(buf));

        }

        else if(!strncmp(buf, "TIME", 4)){
          if(verbose){
            sfwrite(&lock,stdout,"%s%s%s", VERBOSE, buf, DEFAULT);
            // printf("%s%s%s", VERBOSE, buf, DEFAULT);
          }
          clients *client = find_client(connfd);
          time_t current_time = time(NULL);
          int time_duration = current_time - client->login_time;
          char sec[MAXBUF];

          sprintf(sec, "%d", time_duration);
          memset(buf, '\0', strlen(buf));
          strcpy(buf, "EMIT ");
          strcat(buf, sec);
          strcat(buf, " \r\n\r\n");
          Rio_writen(connfd, buf, strlen(buf));
        }

        else if(!strncmp(buf, "BYE", 3)){
          if(verbose){
            // printf("%s%s%s", VERBOSE, buf, DEFAULT);
            sfwrite(&lock,stdout,"%s%s%s", VERBOSE, buf, DEFAULT);
          }
          Rio_writen(connfd, buf, strlen(buf));
          clients *client = find_client(connfd);
          delete(client);
          Close(connfd); //line:conc:echoservers:closeconnfd
          FD_CLR(connfd, &p->read_set); //line:conc:echoservers:beginremove
          p->clientfd[i] = -1;          //line:conc:echoservers:endremove
        }

        else if (!strncmp(buf, "MSG ", 4))
        {
          if(verbose){
            // printf("%s%s%s", VERBOSE, buf, DEFAULT);
            sfwrite(&lock,stdout,"%s%s%s", VERBOSE, buf, DEFAULT);
          }
          int connfd_to, connfd_from, i = 4, count = 0;
          char to[MAXBUF], from[MAXBUF];
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

          connfd_to = find_clientfd_by_name(to);

          connfd_from = find_clientfd_by_name(from);

          //printf("to: %s, from: %s, tocfd: %d, fromcfd: %d\n", to, from, connfd_to, connfd_from);

          if(connfd_to == -1){
            strcpy(buf, "ERR 01 USER NOT AVAILABLE \r\n\r\n");
            Rio_writen(connfd_from, buf, strlen(buf));
          } else {
            Rio_writen(connfd_to, buf, strlen(buf));
            Rio_writen(connfd_from, buf, strlen(buf));
          }


        }

        //Rio_writen(connfd, buf, n); //line:conc:echoservers:endecho
      }

      /* EOF detected, remove descriptor from pool */
      else {
        Close(connfd); //line:conc:echoservers:closeconnfd
        FD_CLR(connfd, &p->read_set); //line:conc:echoservers:beginremove
        p->clientfd[i] = -1;          //line:conc:echoservers:endremove
      }
    }
  }

}

int check_name(char *name){
  pthread_mutex_lock(&client_lock);
  clients *cursor = head;
  while(cursor != NULL){
    if(!strcmp(cursor->name, name)){
      pthread_mutex_unlock(&client_lock);
      return 0;
    }
    cursor = cursor->next;
  }
  pthread_mutex_unlock(&client_lock);
  return 1;
}

int check_username(char *name){
  pthread_mutex_lock(&account_lock);
  accounts *cursor = first;
  while(cursor != NULL){
    if(!strcmp(cursor->name, name)){
      pthread_mutex_unlock(&account_lock);
      return 0;
    }
    cursor = cursor->next;
  }
  pthread_mutex_unlock(&account_lock);
  return 1;
}

int check_user_password(char *password){
  pthread_mutex_lock(&account_lock);
  accounts *cursor = first;
  while(cursor != NULL){
    if(!strcmp(cursor->password, password)){
      pthread_mutex_unlock(&account_lock);
      return 1;
    }
    cursor = cursor->next;
  }
  pthread_mutex_unlock(&account_lock);
  return 0;
}

char* get_salt_by_name (char *name){
  pthread_mutex_lock(&account_lock);
  accounts *cursor = first;
  while(cursor != NULL){
    if(!strcmp(cursor-> name, name)){
      pthread_mutex_unlock(&account_lock);
      return cursor-> salt;
    }
    cursor = cursor -> next;
  }
  pthread_mutex_unlock(&account_lock);
  return NULL;
}

int check_password(char *password){
  int length = 0, symbol = 0, number = 0, uppercase = 0, i;
  for(i=0; i<strlen(password); i++){
    if((password[i]>=33 && password[i]<=47) || (password[i]>=58 && password[i]<=64) || (password[i]>=91 && password[i]<=96) || (password[i]>=123 && password[i]<= 126) ){
      symbol = 1;
    }

    if(password[i]>=65 && password[i]<=90){
      uppercase = 1;
    }

    if(password[i]>=48 && password[i]<=57){
      number = 1;
    }
  }

  if(strlen(password)>=5){
    length = 1;
  }

  if(uppercase && symbol && number && length){
    // printf("%s\n", "Password is valid");
    sfwrite(&lock,stdout,"%s\n", "Password is valid");
    return 1;
  }

  return 0;

}

clients* new_client(char *name, int connfd){
  pthread_mutex_lock(&client_lock);
  clients *new_client = (clients *)malloc(sizeof(clients));
  strcpy(new_client->name, name);
  new_client->connfd = connfd;
  new_client->login_time = time(NULL);
  new_client->next = NULL;
  pthread_mutex_unlock(&client_lock);
  return new_client;
}

void add(clients* new_client){
  pthread_mutex_lock(&client_lock);
  clients *cursor = head;

  if(head == NULL){
    head = new_client;
  }
  else {
    while(cursor->next != NULL){
      cursor = cursor->next;
    }
    cursor->next = new_client;
    new_client->next = NULL;
  }
  pthread_mutex_unlock(&client_lock);

}

void delete(clients *client){
  pthread_mutex_lock(&client_lock);
  if(client == head){
    if(head->next == NULL){
      head = NULL;
      pthread_mutex_unlock(&client_lock);
      return;
    }

    head = head->next;
    free(client);
    pthread_mutex_unlock(&client_lock);
    return;
  }

  clients *prev = head;
  while (prev->next != NULL && prev->next != client) {
    prev = prev->next;
  }

  if(client->next == NULL){
    prev->next = NULL;
  }
  else {
    prev->next = prev->next->next;
  }

  free(client);
  pthread_mutex_unlock(&client_lock);
  return;
}

clients* find_client(int connfd){
  pthread_mutex_lock(&client_lock);
  clients *cursor = head;
  while(cursor != NULL){
    if(cursor->connfd == connfd){
      break;
    }
    cursor = cursor->next;
  }
  pthread_mutex_unlock(&client_lock);
  return cursor;
}
int find_clientfd_by_name(char* name){
  pthread_mutex_lock(&client_lock);
  clients *cursor = head;
  while (cursor != NULL){
    if (!strcmp(name,cursor->name)){
      pthread_mutex_unlock(&client_lock);
      return cursor-> connfd;
    }
    cursor = cursor -> next;
  }
  pthread_mutex_unlock(&client_lock);
  return -1;
}

accounts* new_account(char *name, char *password, char* salt){
  pthread_mutex_lock(&account_lock);
  accounts *new_account = (accounts *)malloc(sizeof(accounts));
  strcpy(new_account->name, name);
  strcpy(new_account->password, password);
  strcpy(new_account->salt, salt);
  new_account->next = NULL;
    pthread_mutex_unlock(&account_lock);
  return new_account;
}
void add_account(accounts *new_account){
    pthread_mutex_lock(&account_lock);
  accounts *cursor = first;

  if(first == NULL){
    first = new_account;
  } else {
    while(cursor->next != NULL){
      cursor = cursor->next;
    }
    cursor->next = new_account;
    new_account->next = NULL;
  }

    pthread_mutex_unlock(&account_lock);
}

void display_cmd(){
  sfwrite(&lock,stdout,"/users:\t\t%s\n", "Display the list of current logged in users.");
  sfwrite(&lock,stdout,"/help:\t\t%s\n", "List all the available commands on server side.");
  sfwrite(&lock,stdout,"/shutdown:\t\t%s\n", "Server will be terminated and disconnect all connected users.");

  // printf("/users:\t\t%s\n", "Display the list of current logged in users.");
  // printf("/help:\t\t%s\n", "List all the available commands on server side.");
  // printf("/shutdown:\t\t%s\n", "Server will be terminated and disconnect all connected users.");
}

void display_user(){
    pthread_mutex_lock(&client_lock);
  clients *cursor = head;

  while(cursor != NULL){
    // printf("%s\n", cursor->name);

    sfwrite(&lock,stdout,"%s\n", cursor->name);

    cursor = cursor->next;
  }
pthread_mutex_unlock(&client_lock);
}

void display_accts_info(){
  pthread_mutex_lock(&account_lock);
  accounts *cursor = first;

  while(cursor != NULL){
    // printf("%s\n", cursor->name);
    sfwrite(&lock,stdout,"%s\n", cursor->name);
    cursor = cursor->next;
  }
pthread_mutex_unlock(&account_lock);
}

void ctrlC_handler(){

  char buf[MAXBUF];
  int i,connfd;
  for (i = 0; (i <= fd_pool.maxi); i++) {
    connfd = fd_pool.clientfd[i];
    memset(buf, '\0', strlen(buf));
    strcpy(buf, "BYE \r\n\r\n");
    Rio_writen(connfd, buf, strlen(buf));
    Close(connfd); //line:conc:echoservers:closeconnfd
    FD_CLR(connfd, &fd_pool.read_set); //line:conc:echoservers:beginremove
    fd_pool.clientfd[i] = -1;          //line:conc:echoservers:endremove
  }
  sqlite3_close(db);
  close(listenfd);
  exit(1);

}

login_queue* new_login(int *fdt)
{
  login_queue* new_request = (login_queue*) malloc (sizeof(login_queue));
  new_request -> fd = fdt;
  new_request -> next = NULL;
  return new_request;
}

void add_to_queue(login_queue* request)
{
  login_queue * cursor = top;
  if (top == NULL){
    top = request;
  }
  else{
    while(cursor -> next!= NULL){
      cursor = cursor -> next;
    }
    cursor -> next = request;
    request -> next = NULL;
  }
}

void delete_from_queue (login_queue* request)
{
  if (request == top) {
    if (top -> next == NULL){
      top = NULL;
      return;
    }
    top = top->next;
    free(request);
    return;
  }

  login_queue* prev = top;
  while (prev->next != NULL && prev -> next != request){
    prev = prev->next;
  }
  if (request-> next ==NULL){
    prev->next = NULL;
  }
  else{
    prev-> next = prev->next->next;
  }
  free (request);
  return;
}

login_queue* handle_request(){
  return top;
}
