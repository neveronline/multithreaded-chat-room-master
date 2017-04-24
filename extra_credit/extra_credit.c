#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXBUF 128
#define MAXLINE 128

#define RESULT      "\x1B[1;34m"
#define CMD         "\x1B[1;31m"
#define DEFAULT     "\x1B[0m"

int sort_by_time(const void *one, const void *two);
int sort_by_name(const void *one, const void *two);
int sort_by_event(const void *one, const void *two);
int sort_by_cmd(const void *one, const void *two);
int filter_by_name(char *line[], char *name);
int filter_by_event(char *line[], char *event);
int filter_by_time(char *line[], char *from, char *to);
int filter_by_result(char *line[], char *result);
int filter_by_err(char *line[], char *err);
void remix_time(char *buf, char *buf_remix);
int search_by_keywords(char *line[], char *keywords);


int main (int argc, char **argv){
  FILE *fp;
  int i, line_num;
  char *line[MAXLINE], file[MAXBUF], buf[MAXBUF], name[MAXBUF], event[MAXBUF], from[MAXBUF], to[MAXBUF], result[MAXBUF], err[MAXBUF], keywords[MAXBUF];

  if(argc < 2){
    printf("%s\n", "Usage: ./tool [FILE PATH]");
    exit(0);
  }

  strcpy(file, argv[1]);

  fp = fopen(file, "r");

  for(i=0; i<MAXLINE; i++){
    line[i] = malloc(MAXBUF);
    memset(line[i], '\0', MAXBUF);
  }

  i=0; line_num=0;
  while(fgets(line[i], MAXLINE, fp)){
    i++;
    line_num++;
  }

  while(1){
    printf("%s\n", "*********************************************************************************");
    printf("%s\n\n","Available Command: ");
    printf("%s%s%s\n", CMD, "Sort: ", DEFAULT);
    printf("\t- %s%s%s%s\n", CMD, "sbta: ", DEFAULT, "Sort by time in ascending order");
    printf("\t- %s%s%s%s\n", CMD, "sbtd: ", DEFAULT, " Sort by time in descending order");
    printf("\t- %s%s%s%s\n", CMD, "sbna: ", DEFAULT, "Sort by name in ascending order");
    printf("\t- %s%s%s%s\n", CMD, "sbnd: ", DEFAULT, "Sort by name in descending order");
    printf("\t- %s%s%s%s\n", CMD, "sbea: ", DEFAULT, "Sort by event in ascending order");
    printf("\t- %s%s%s%s\n", CMD, "sbed: ", DEFAULT, "Sort by event in descending order");
    printf("\t- %s%s%s%s\n", CMD, "sbca: ", DEFAULT, "Sort by command in ascending order (Available after \"fbe [CMD]\")");
    printf("\t- %s%s%s%s\n", CMD, "sbcd: ", DEFAULT, "Sort by command in descending order (Available after \"fbe [CMD]\")");
    printf("\n");

    printf("%s%s%s%s\n",CMD, "Filter: ", DEFAULT, "(Do not include \"[]\" when you enter the command)\n");
    printf("\t- %s%s%s%s\n",  CMD, "fbt [mm/dd/yy-hh/mmAM/PM] to [mm/dd/yy-hh/mmAM/PM]: ", DEFAULT, "Filter by time");
    printf("\t- %s%s%s%s\n",  CMD, "fbn [name]: ", DEFAULT, "Filter by name");
    printf("\t- %s%s%s%s\n",  CMD, "fbe [LOGIN/MSG/CMD/ERR/LOGOUT]: ", DEFAULT, "Filter by event");
    printf("\t- %s%s%s%s\n",  CMD, "fbr [success/fail]: ", DEFAULT, "Filter by result");
    printf("\t- %s%s%s%s\n",  CMD, "fber [ERR 00/01/02/100]: ", DEFAULT, "Filter by error code");
    printf("\n");

    printf("%s%s%s%s\n",CMD, "Reset: ", DEFAULT, "(Since the result set generated is based on your last result set, this commmand is used for reset the result set same as the log file)\n");
    printf("\t- %s%s%s%s\n",  CMD, "reset: ", DEFAULT, "reinitialize the result set");
    printf("\n");

    printf("%s%s%s\n", CMD, "Search: ", DEFAULT);
    printf("\t- %s%s%s%s\n",  CMD, "search [KEYWORDS]: ", DEFAULT, "Return the line of log that includes the keywords");
    printf("\n");

    printf("%s%s%s\n", CMD, "Watch: ", DEFAULT);
    printf("\t- %s%s%s%s\n",  CMD, "watch: ", DEFAULT, "Watch the file for live updates");
    printf("\n");

    printf("%s%s%s\n", CMD, "Exit: ", DEFAULT);
    printf("\t- %s%s%s%s\n",  CMD, "exit: ", DEFAULT, "Exit the program");
    printf("%s\n", "*********************************************************************************");


    printf("Please enter the command: ");
    memset(buf, '\0', MAXBUF);
    fgets(buf, MAXBUF, stdin);
    buf[strlen(buf)-1] = '\0'; //remove '\n'



    if(!strcmp(buf, "sbta")){
      qsort (line, line_num, sizeof (char *), sort_by_time);
      for (i = 0; i < line_num; i++) {
          printf ("%s%d: %s%s", RESULT, i, line[i], DEFAULT);
      }
    }

    else if(!strcmp(buf, "sbna")){
      qsort (line, line_num, sizeof (char *), sort_by_name);
      for (i = 0; i < line_num; i++) {
          printf ("%s%d: %s%s", RESULT, i, line[i], DEFAULT);
      }
    }

    else if(!strcmp(buf, "sbea")){
      qsort (line, line_num, sizeof (char *), sort_by_event);
      for (i = 0; i < line_num; i++) {
          printf ("%s%d: %s%s", RESULT, i, line[i], DEFAULT);
      }
    }

    else if(!strcmp(buf, "sbca")){
      qsort (line, line_num, sizeof (char *), sort_by_cmd);
      for (i = 0; i < line_num; i++) {
          printf ("%s%d: %s%s", RESULT, i, line[i], DEFAULT);
      }
    }

    else if(!strcmp(buf, "reset")){
      fp = fopen(file, "r");

      for(i=0; i<MAXLINE; i++){
        memset(line[i], '\0', MAXBUF);
      }

      i=0; line_num=0;
      while(fgets(line[i], MAXLINE, fp)){
        i++;
        line_num++;
      }

      printf("%s%s%s\n", RESULT, "Reset Completed!", DEFAULT);
    }

    else if(!strncmp(buf, "fbn ", 4)){
      memset(name, '\0', MAXBUF);
      strcpy(name, &buf[4]);
      line_num = filter_by_name(line, name);
      for (i = 0; i < MAXLINE && line[i][0] != '\0'; i++) {
          printf ("%s%d: %s%s", RESULT, i, line[i], DEFAULT);
      }
    }

    else if(!strncmp(buf, "fbe ", 4)){
      memset(event, '\0', MAXBUF);
      strcpy(event, &buf[4]);
      line_num = filter_by_event(line, event);
      for (i = 0; i < MAXLINE && line[i][0] != '\0'; i++) {
          printf ("%s%d: %s%s", RESULT, i, line[i], DEFAULT);
      }
    }

    else if(!strncmp(buf, "fbt ", 4)){
      memset(from, '\0', MAXBUF);
      memset(to, '\0', MAXBUF);
      strncpy(from, &buf[4], 16);
      strncpy(to, &buf[24], 16);
      line_num = filter_by_time(line, from, to);
      for (i = 0; i < MAXLINE && line[i][0] != '\0'; i++) {
          printf ("%s%d: %s%s", RESULT, i, line[i], DEFAULT);
      }
    }

    else if(!strncmp(buf, "fbr ", 4)){
      memset(result, '\0', MAXBUF);
      strcpy(result, &buf[4]);
      line_num = filter_by_result(line, result);
      for (i = 0; i < MAXLINE && line[i][0] != '\0'; i++) {
          printf ("%s%d: %s%s", RESULT, i, line[i], DEFAULT);
      }
    }

    else if(!strncmp(buf, "fber ", 5)){
      memset(err, '\0', MAXBUF);
      strcpy(err, &buf[5]);
      line_num = filter_by_err(line, err);
      for (i = 0; i < MAXLINE && line[i][0] != '\0'; i++) {
          printf ("%s%d: %s%s", RESULT, i, line[i], DEFAULT);
      }
    }

    else if(!strncmp(buf, "search ", 7)){
      memset(keywords, '\0', MAXBUF);
      strcpy(keywords, &buf[7]);
      line_num = search_by_keywords(line, keywords);
      if(line_num == 0){
        printf("%s%s%s\n", RESULT, "Sorry, no results are found.", DEFAULT);
      }
      for (i = 0; i < MAXLINE && line[i][0] != '\0'; i++) {
          printf ("%s%d: %s%s", RESULT, i, line[i], DEFAULT);
      }
    }

    else if(!strcmp(buf, "exit")){
      break;
    }

  }

  fclose(fp);
  exit(0);
}

int sort_by_time(const void *one, const void *two){
  char *a = *(char **) one;
  char *b = *(char **) two;

  char a_remix[MAXBUF], b_remix[MAXBUF];
  remix_time(a, a_remix);
  remix_time(b, b_remix);

  return strcmp(a_remix, b_remix);
}

int sort_by_name(const void *one, const void *two){
  char *a = *(char **) one;
  char *b = *(char **) two;

  char a_name[MAXBUF], b_name[MAXBUF];
  int i, length;
  memset(a_name, '\0', MAXBUF);
  memset(b_name, '\0', MAXBUF);

  i=0, length=0;
  while(a[i] != ' '){
    i++;
  }
  i++; //start of name

  while(a[i] != ','){
    i++;
    length++;
  }

  i = i-length;
  strncpy(a_name, &a[i], length);

  i=0, length=0;
  while(b[i] != ' '){
    i++;
  }
  i++; //start of name

  while(b[i] != ','){
    i++;
    length++;
  }

  i = i-length;
  strncpy(b_name, &b[i], length);

  return strcmp(a_name, b_name);
}

int sort_by_event(const void *one, const void *two){
  char *a = *(char **) one;
  char *b = *(char **) two;

  char a_event[MAXBUF], b_event[MAXBUF];
  int i, length;
  memset(a_event, '\0', MAXBUF);
  memset(b_event, '\0', MAXBUF);

  i=0, length=0;
  while(a[i] != ' '){
    i++;
  }
  i++; //start of name
  while(a[i] != ' '){
    i++;
  }
  i++; //start of event

  while(a[i] != ','){
    i++;
    length++;
  }

  i = i-length;
  strncpy(a_event, &a[i], length);

  i=0, length=0;
  while(b[i] != ' '){
    i++;
  }
  i++; //start of name
  while(b[i] != ' '){
    i++;
  }
  i++; //start of event

  while(b[i] != ','){
    i++;
    length++;
  }

  i = i-length;
  strncpy(b_event, &b[i], length);

  return strcmp(a_event, b_event);
}

int sort_by_cmd(const void *one, const void *two){
  char *a = *(char **) one;
  char *b = *(char **) two;

  char a_cmd[MAXBUF], b_cmd[MAXBUF];
  int i, length;
  memset(a_cmd, '\0', MAXBUF);
  memset(b_cmd, '\0', MAXBUF);

  i=0, length=0;
  while(a[i] != ' '){
    i++;
  }
  i++; //start of name
  while(a[i] != ' '){
    i++;
  }
  i++; //start of event
  while(a[i] != ' '){
    i++;
  }
  i++; //start of cmd

  while(a[i] != ','){
    i++;
    length++;
  }

  i = i-length;
  strncpy(a_cmd, &a[i], length);

  i=0, length=0;
  while(b[i] != ' '){
    i++;
  }
  i++; //start of name
  while(b[i] != ' '){
    i++;
  }
  i++; //start of event
  while(b[i] != ' '){
    i++;
  }
  i++; //start of cmd

  while(b[i] != ','){
    i++;
    length++;
  }

  i = i-length;
  strncpy(b_cmd, &b[i], length);

  return strcmp(a_cmd, b_cmd);

}

int filter_by_name(char *line[], char *name){
  char *tmp[MAXLINE], username[MAXBUF];
  int i, j, length, count=0;

  for(i=0; i<MAXLINE; i++){
    tmp[i] = malloc(MAXBUF);
    memset(tmp[i], '\0', MAXBUF);
  }

  for(i=0; i<MAXLINE && line[i][0] != '\0'; i++){
    j=0, length = 0;
    while(line[i][j] != ' '){
      j++;
    }
    j++; //start of name

    while(line[i][j] != ','){
      j++;
      length++;
    }

    j = j-length;
    memset(username, '\0', MAXBUF);
    strncpy(username, &line[i][j], length);

    if(!strcmp(name, username)){
      strcpy(tmp[count], line[i]);
      count++;
    }
  }

  for(i=0; i<MAXLINE; i++){ //clear the line buf
    memset(line[i], '\0', MAXBUF);
  }

  for(i=0; i<MAXLINE && tmp[i][0] != '\0'; i++){ //copy tmp to line
    strcpy(line[i], tmp[i]);
  }

  return i; //return line num
}

int filter_by_event(char *line[], char *event){
  char *tmp[MAXLINE], event_buf[MAXBUF];
  int i, j, length, count=0;

  for(i=0; i<MAXLINE; i++){
    tmp[i] = malloc(MAXBUF);
    memset(tmp[i], '\0', MAXBUF);
  }

  for(i=0; i<MAXLINE && line[i][0] != '\0'; i++){
    j=0, length = 0;
    while(line[i][j] != ' '){
      j++;
    }
    j++; //start of name

    while(line[i][j] != ' '){
      j++;
    }
    j++; //start of event

    while(line[i][j] != ','){
      j++;
      length++;
    }

    j = j-length;
    memset(event_buf, '\0', MAXBUF);
    strncpy(event_buf, &line[i][j], length);

    if(!strcmp(event, event_buf)){
      strcpy(tmp[count], line[i]);
      count++;
    }
  }

  for(i=0; i<MAXLINE; i++){ //clear the line buf
    memset(line[i], '\0', MAXBUF);
  }

  for(i=0; i<MAXLINE && tmp[i][0] != '\0'; i++){ //copy tmp to line
    strcpy(line[i], tmp[i]);
  }

  return i; //return line num
}

int filter_by_time(char *line[], char *from, char *to){
  char *tmp[MAXLINE], time_buf[MAXBUF];
  int i, j, length, count=0;

  for(i=0; i<MAXLINE; i++){
    tmp[i] = malloc(MAXBUF);
    memset(tmp[i], '\0', MAXBUF);
  }

  for(i=0; i<MAXLINE && line[i][0] != '\0'; i++){
    j=0, length = 0;

    while(line[i][j] != ','){
      j++;
      length++;
    }

    j = j-length;
    memset(time_buf, '\0', MAXBUF);
    strncpy(time_buf, &line[i][j], length);

    char time_buf_remix[MAXBUF];
    char from_remix[MAXBUF];
    char to_remix[MAXBUF];

    remix_time(time_buf, time_buf_remix);
    remix_time(from, from_remix);
    remix_time(to, to_remix);

    if(strcmp(time_buf_remix, from_remix) > 0 && strcmp(time_buf_remix, to_remix) < 0){
      strcpy(tmp[count], line[i]);
      count++;
    }
  }

  for(i=0; i<MAXLINE; i++){ //clear the line buf
    memset(line[i], '\0', MAXBUF);
  }

  for(i=0; i<MAXLINE && tmp[i][0] != '\0'; i++){ //copy tmp to line
    strcpy(line[i], tmp[i]);
  }

  return i; //return line num
}

void remix_time(char *buf, char *buf_remix){
  memset(buf_remix, '\0', MAXBUF);
  strncpy(buf_remix, &buf[6], 2);
  strcat(buf_remix, "/");
  strncat(buf_remix, buf, 5);
  strncat(buf_remix, &buf[8], 1);
  strncat(buf_remix, &buf[14], 2);
  strncat(buf_remix, &buf[9], 5);
}

int filter_by_result(char *line[], char *result){
  char *tmp[MAXLINE], result_buf[MAXBUF];
  int i, j, length, count=0, k;

  for(i=0; i<MAXLINE; i++){
    tmp[i] = malloc(MAXBUF);
    memset(tmp[i], '\0', MAXBUF);
  }

  for(i=0; i<MAXLINE && line[i][0] != '\0'; i++){
    j=0, length = 0;
    for(k=0; k<4; k++){
      while(line[i][j] != ' '){
        j++;
      }
      j++;
    }

    while(line[i][j] != ','){
      j++;
      length++;
    }

    j = j-length;
    memset(result_buf, '\0', MAXBUF);
    strncpy(result_buf, &line[i][j], length);

    if(!strcmp(result, result_buf)){
      strcpy(tmp[count], line[i]);
      count++;
    }
  }

  for(i=0; i<MAXLINE; i++){ //clear the line buf
    memset(line[i], '\0', MAXBUF);
  }

  for(i=0; i<MAXLINE && tmp[i][0] != '\0'; i++){ //copy tmp to line
    strcpy(line[i], tmp[i]);
  }

  return i; //return line num
}

int filter_by_err(char *line[], char *err){
  char *tmp[MAXLINE], err_buf[MAXBUF];
  int i, j, count=0, k;

  for(i=0; i<MAXLINE; i++){
    tmp[i] = malloc(MAXBUF);
    memset(tmp[i], '\0', MAXBUF);
  }

  for(i=0; i<MAXLINE && line[i][0] != '\0'; i++){
    j=0;
    for(k=0; k<5; k++){
      while(line[i][j] != ' '){
        j++;
      }
      j++;
    }

    memset(err_buf, '\0', MAXBUF);
    strncpy(err_buf, &line[i][j], 6);

    if(!strncmp(err, err_buf, 6)){
      strcpy(tmp[count], line[i]);
      count++;
    }
  }

  for(i=0; i<MAXLINE; i++){ //clear the line buf
    memset(line[i], '\0', MAXBUF);
  }

  for(i=0; i<MAXLINE && tmp[i][0] != '\0'; i++){ //copy tmp to line
    strcpy(line[i], tmp[i]);
  }

  return i; //return line num
}

int search_by_keywords(char *line[], char *keywords){
  char *tmp[MAXLINE], *ret;
  int i, count=0;

  for(i=0; i<MAXLINE; i++){
    tmp[i] = malloc(MAXBUF);
    memset(tmp[i], '\0', MAXBUF);
  }

  for(i=0; i<MAXLINE && line[i][0] != '\0'; i++){
    ret = strstr(line[i], keywords);

    if(ret != NULL){
      strcpy(tmp[count], line[i]);
      count++;
    }
  }

  for(i=0; i<MAXLINE; i++){ //clear the line buf
    memset(line[i], '\0', MAXBUF);
  }

  for(i=0; i<MAXLINE && tmp[i][0] != '\0'; i++){ //copy tmp to line
    strcpy(line[i], tmp[i]);
  }

  return i; //return line num
}
