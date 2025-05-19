CFLAGS ?= -O3

jfetch: jfetch.c
	${CC} ${CFLAGS} $^ -o $@

clean:
	rm -f jfetch
