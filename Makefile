CC = gcc
CFLAGS = -O3 -marm -mcpu=cortex-a35 -mfpu=neon-vfpv4 -mfloat-abi=hard -Wall -Wextra -rdynamic
LDFLAGS = -ldl -lSDL2 -lEGL -lGLESv2 -lm -lpthread

TARGET = nfs_loader
SRCS = loader.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
