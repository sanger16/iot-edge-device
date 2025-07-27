CC=gcc
CFLAGS=-I. -pthread -ldl
DEPS = edgelib/edgelib.h
OBJ = edge.o edgelib/edgelib.o sqlite3/sqlite3.o
LIBS=-lpaho-mqtt3cs -lm


%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

edge: $(OBJ)
	$(CC) -o $@ $^ $(LIBS) $(CFLAGS)