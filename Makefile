CC = gcc
CFLAGS = -O1 -g -marm -mcpu=cortex-a35 -mfpu=neon-vfpv4 -mfloat-abi=hard \
         -Wall -Wextra -rdynamic -D_GNU_SOURCE -fPIC \
         -fno-stack-protector -U_FORTIFY_SOURCE \
         -Wno-cast-function-type \
         -funwind-tables -fasynchronous-unwind-tables \
         -fvisibility=hidden
LDFLAGS = -ldl -lSDL2 -lEGL -lGLESv2 -lm -lpthread \
          -Wl,--export-dynamic -Wl,-E

TARGET = nfs_loader
SRCS = loader.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
