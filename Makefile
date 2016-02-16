LIBNL3 := -I/usr/local/include/libnl3 -L/usr/local/lib -lnl-3 -lnl-genl-3
LIBPCAP := -lpcap 
TIMER := -lrt
ELF := a.out

all: $(ELF)
$(ELF) : main.c airkiss.c wifi_scan.c
	gcc $^ -o $@ $(LIBNL3) $(LIBPCAP) $(TIMER) -g

clean:
	rm -f  $(ELF)
