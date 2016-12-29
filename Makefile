ELF := airkiss
SRCS := main.c capture/common.c capture/osdep.c capture/linux.c capture/radiotap/radiotap-parser.c  utils/wifi_scan.c airkiss.c
OBJS = $(patsubst %.c,%.o,$(SRCS))
LIBIW := -liw
AIRKISS := -L./libs -lairkiss_log
TIMER := -lrt
CC:=gcc
CCFLAGS := -c -g -I/usr/local/include/libnl3

all: $(ELF)
$(ELF) : $(OBJS)
	$(CC) $^ -o $@ $(LIBIW) $(TIMER) 
$(OBJS):%.o:%.c
	$(CC) $(CCFLAGS) $< -o $@

clean:
	rm -f  $(ELF) $(OBJS)
