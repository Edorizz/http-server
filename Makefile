.POSIX:
.SUFFIXES:

CC     := gcc
CFLAGS := -std=gnu99 -pedantic -Wall
RM     := rm -f

http-server: http-server.c
	$(CC) $(CFLAGS) $^ -o $@

.PHONY:
clean:
	$(RM) http-server

