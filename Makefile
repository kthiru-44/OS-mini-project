CC     = gcc
CFLAGS = -Wall -Wextra -g -std=c99 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
LDLIBS = -pthread -lrt
SRN   ?= PES2UG25CS586

SRCS = main.c scheduler.c logger.c ipc.c banker.c worker.c
OBJS = $(SRCS:.c=.o)

campuscompute: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c project.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) campuscompute campuscompute.log $(SRN).zip

zip: clean
	zip -r $(SRN).zip $(SRCS) project.h Makefile samples README.md
	unzip -l $(SRN).zip
	unzip -t $(SRN).zip

.PHONY: clean zip
