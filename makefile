
all:
	mkdir -p bin
	gcc -std=c11 -Wall -Wextra -pedantic -Werror  -lfuse src/redirfs.c -o bin/redirfs

clean:
	rm -rf bin
