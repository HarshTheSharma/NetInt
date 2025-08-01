CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -lm

TARGET = its-mpc
SRCS = ITS-MPC.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)


clean:
	rm -f $(TARGET)