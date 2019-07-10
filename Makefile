all: test

CFLAGS=-I steamworks/public/ -fmax-errors=20
LDFLAGS=-L steamworks/redistributable_bin/win64/ -l steam_api64

test: test.cpp
	g++ $(CFLAGS) test.cpp -o test $(LDFLAGS)
