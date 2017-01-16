CFLAGS= -c -ggdb3 -Wall -std=gnu99
LINKFLAGS= -liw
all: main.out
main.o:main.c
	gcc $(CFLAGS) $< -o $@
wifi_scan.o: ../../utils/wifi_scan.c
	gcc $(CFLAGS) $< -o $@

main.out: main.o wifi_scan.o
	gcc -o $@ $^ $(LINKFLAGS)

clean:
	rm -f *.out *.o
.PHONY: all clean
