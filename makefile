BIN := steam-shutdown

FILES += src/main.c

all: clean
	cc $(FILES) -o $(BIN)

clean:
	rm -f $(BIN)
