#CFLAGS = -Wall -Wextra -Wformat=2 -Wconversion -Wimplicit-fallthrough -Werror
#CFLAGS = -Wall -Wextra -Wformat=2 -Wconversion -Wimplicit-fallthrough

all: nstest

.PHONY: clean
clean:
	rm -f *.o nstest

nstest: main.o
	gcc -o $@ $^

main.o: main.c
	gcc $(CFLAGS) -o $@ -c $^
