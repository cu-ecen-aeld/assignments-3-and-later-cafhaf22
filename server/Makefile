ifeq ($(strip $(CROSS_COMPILE)),)
	CC = gcc
else
	CC := ${CROSS_COMPILE}gcc
endif

SRC := aesdsocket.c
TARGET = aesdsocket
OBJS := $(SRC:.c=.o)

all: $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o $(TARGET) $(LDFLAGS)

clean:
	-rm -f *.o $(TARGET) *.elf *.map
