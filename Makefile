ELF = airkiss
SRCS = main.c 
SRCS += capture/common.c capture/osdep.c capture/linux.c capture/radiotap/radiotap-parser.c
SRCS += utils/wifi_scan.c airkiss.c
OBJS = $(patsubst %.c,%.o,$(SRCS))

LIBIW = -liw
TIMER = -lrt
AIRKISS = -L./libs -lairkiss_log

CC = gcc
CCFLAGS = -c -g -Wall -Wno-unused-but-set-variable

all: $(ELF)
$(ELF) : $(OBJS)
	$(CC) $^ -o $@ $(LIBIW) $(TIMER) 
$(OBJS):%.o:%.c
	$(CC) $(CCFLAGS) $< -o $@

clean:
	rm -f  $(ELF) $(OBJS)

.PHONY: all clean
