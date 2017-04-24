CFLAGS = -Wall -Werror
BINS = tool

all: $(BINS)

tool: extra_credit.o
	gcc $(CFLAGS) $^ -o $@

extra_credit.o: extra_credit.c
	gcc $(CFLAGS) -c $^

clean:
	rm -f *.o $(BINS)
