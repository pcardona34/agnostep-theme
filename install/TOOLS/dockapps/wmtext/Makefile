EXEC	= wmtext
SRCS	= src/wmtext.c
OBJS	= objs/wmtext.o

CC	= gcc
CFLAGS	= -Wall -ansi

BIN_DIR	= /usr/local/bin

all: depend
	$(MAKE) $(EXEC)

$(EXEC): $(OBJS)
	$(CC) -o $(EXEC) $(OBJS) -lX11 -ldockapp

objs/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

-include depend

depend:
	$(CC) -I. -MM $(SRCS) -MF depend
	@# prepend objs/ before object targets
	sed -i -e 's;^\([a-z\.]\+\.o:\);objs/\1;' depend

install:
	install -o root -g root -m 755 $(EXEC) $(BIN_DIR)

uninstall:
	rm -f $(BIN_DIR)/$(EXEC)

clean:
	rm -f objs/*.o $(EXEC) depend *~ */*~

.PHONY: install uninstall clean
