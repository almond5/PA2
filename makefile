CC = gcc
TARGET = chash
SRCS = hashdb.c chash.c rwlock.c
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.h)
LIBS = -lpthread

$(TARGET): $(OBJS)
	$(CC) $^ $(LIBS) -o $@
	rm -f $(OBJS)

%.o: %.c $(DEPS)
	$(CC) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
