EXE=server_main client_main
CC=clang++
FLAG=-lhiredis -levent -lpthread -O2 -g -Wall -Wextra -fstandalone-debug -fcolor-diagnostics -std=c++20
OBJ=redis_publisher.o publisher.o redis_subscriber.o subscriber.o

all:$(EXE)

$(EXE):$(OBJ)
	$(CC) -o publisher redis_publisher.o publisher.o $(FLAG)
	$(CC) -o subscriber redis_subscriber.o subscriber.o $(FLAG)

redis_publisher.o:redis_publisher.hpp redis_publisher.cxx
	$(CC) -c redis_publisher.cxx $(FLAG)
redis_subscriber.o:redis_subscriber.hpp redis_subscriber.cxx
	$(CC) -c redis_subscriber.cxx $(FLAG)

publisher.o:publisher.cxx
	$(CC) -c publisher.cxx $(FLAG)

subscriber.o:subscriber.cxx
	$(CC) -c subscriber.cxx $(FLAG)

clean:
	rm publisher subscriber *.o