CPPFLAGS = -g  

.PHONY: all build run clean

all: build

build: server subscriber

server: server.c
	gcc $(CPPFLAGS) -o server server.c -lm

subscriber: subscriber.c
	gcc $(CPPFLAGS) -o subscriber subscriber.c

run: server subscriber
	./server arg1 &
	./subscriber arg2 arg3 arg4

clean:
	rm -f server subscriber
