CC=gcc
CFLAGS=-I.
OBJ=md_parser.o
DEPS=

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

md_parser: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

