CC=gcc
TARGET=libmpt.a
SRCS=$(wildcard *.c)
OBJS=$(SRCS:%.c=%.o)
LIBS=-pthread
CFLAGS=-DMPT_SERVER -I../include -g

.PHONY: clean

$(TARGET): $(OBJS)
	@echo "  AR    $(TARGET)"
	@$(AR) cr $(TARGET) $(OBJS)

%.o: %.c
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@ -Wall

clean:
	rm -f $(OBJS) *~
	rm -f $(TARGET)
