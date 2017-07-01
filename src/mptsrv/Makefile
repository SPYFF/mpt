TARGET=../../mptsrv
SRCS=$(wildcard *.c)
OBJS=$(SRCS:%.c=%.o)
LIBS=-pthread -lcrypto
#CFLAGS=-DMPT_SERVER -I../include -O2 -static
CFLAGS=-DMPT_SERVER -I../include -O2

.PHONY: clean exec libmpt.a

$(TARGET): libmpt.a $(OBJS)
	@echo "  BUILD $(TARGET)"
	@$(CC) $(CFLAGS) $(OBJS) libmpt.a $(LIBS) -o $(TARGET)

%.o: %.c
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@ -Wall

libmpt.a:
	@test ! -d ../mptlib || $(MAKE) -C ../mptlib $@
	@test ! -d ../mptlib || cp ../mptlib/$@ .

clean: clean-mptlib
	rm -f $(OBJS) libmpt.a *~
	rm -f $(TARGET)

clean-mptlib:
	@test ! -d $(@:clean-%=../%) || $(MAKE) -C $(@:clean-%=../%) clean
