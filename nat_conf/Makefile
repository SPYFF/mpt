SHELL=/bin/bash
TARGET=nat_conf
SRCS=$(wildcard *.c)
OBJS=$(SRCS:%.c=%.o)

.PHONY: clean exec

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "  BUILD $(TARGET)"
	@$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(TARGET)

%.o: %.c
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@ -Wall

exec: $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $(TARGET)

clean:
	rm -f $(OBJS)
	rm -f $(TARGET)
