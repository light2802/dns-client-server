# Makefile for dns_server

version := $(shell sh -c 'cat VERSION')

CC = gcc
RM = rm -f
CP = cp -f

CFLAGS = -O2 -Wall -DVERSION=\"${version}\" -DNDEBUG
LDFLAGS = -s

TARGET = server.out
INCLUDES = $(wildcard *.h embed/*.h)
SOURCES = $(wildcard *.c embed/*.c)
OBJS = $(patsubst %.c,%.o,$(SOURCES))

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJS): $(SOURCES) $(INCLUDES)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

clean:
	$(RM) *.o embed/*.o *~ $(TARGET)
