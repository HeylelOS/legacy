CC=clang
CFLAGS=-g -Wall -fPIC
GETTY_SRC=$(wildcard src/getty/*.c)
LOGIN_SRC=$(wildcard src/login/*.c)
GETTY=build/bin/getty
LOGIN=build/bin/login

BUILDDIR=build

.PHONY: all clean

all: $(GETTY) $(LOGIN)

clean:
	rm -rf $(BUILDDIR)

$(BUILDDIR):
	mkdir -p $@/bin

$(GETTY): $(GETTY_SRC) $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $(GETTY_SRC)

$(LOGIN): $(LOGIN_SRC) $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $(LOGIN_SRC)


