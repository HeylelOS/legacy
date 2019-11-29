CC=clang
CFLAGS=-g -Wall -fPIC
GETTY_SRC=$(wildcard src/getty/*.c)
GETTY=build/bin/getty

BUILDDIR=build

.PHONY: all clean

all: $(GETTY)

clean:
	rm -rf $(BUILDDIR)

$(BUILDDIR):
	mkdir -p $@/bin

$(GETTY): $(GETTY_SRC) $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $(GETTY_SRC)


