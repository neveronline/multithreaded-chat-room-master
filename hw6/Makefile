CFLAGS = -Wall -Werror -g
BINS = client server chat tool watch

all: $(BINS)
chat: csapp.o chat.o -lpthread
server: server.o csapp.o sfwrite.o -lpthread -lsqlite3 -lssl -lcrypto
tool: tool.o
client: client.o csapp.o sfwrite.o -lpthread
watch: watch.o

	gcc $(CFLAGS) $^ -o $@



server.o: server.c
	gcc $(CFLAGS) -c $^

client.o: client.c
	gcc $(CFLAGS) -c $^

csapp.o: csapp.c
	gcc $(CFLAGS) -c $^

chat.o: chat.c
	gcc $(CFLAGS) -c $^

sfwrite.o: sfwrite.c
	gcc $(CFLAGS) -c $^

tool.o: tool.c
	gcc $(CFLAGS) -c $^

watch.o: watch.c
	gcc $(CFLAGS) -c $^

clean:
	rm -f *.o $(BINS)
