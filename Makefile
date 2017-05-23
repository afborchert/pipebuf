CFiles := $(wildcard *.c)
Targets := $(patsubst %.c,%,$(CFiles))

.PHONY:		all clean

CC :=		gcc
CFLAGS :=	-std=gnu11 -Wall

all:		$(Targets)
clean:
		rm -f $(Targets)
