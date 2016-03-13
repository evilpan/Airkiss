ELF := airkiss
SRCS := main.c  osdep.c linux.c radiotap/radiotap-parser.c common.c wifi_scan.c airkiss.c
OBJS = $(patsubst %.c,%.o,$(SRCS))
LIBNL3 :=  -L/usr/local/lib -lnl-3 -lnl-genl-3
LIBPCAP := -lpcap 
AIRKISS := -L./libs -lairkiss_log
TIMER := -lrt
CC:=gcc
CCFLAGS := -c -g -I/usr/local/include/libnl3

all: $(ELF)
$(ELF) : $(OBJS)
	$(CC) $^ -o $@ $(LIBNL3) $(LIBPCAP) $(TIMER) 
$(OBJS):%.o:%.c
	$(CC) $(CCFLAGS) $< -o $@

clean:
	rm -f  $(ELF) $(OBJS)
