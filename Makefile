# Output binary name
bin=swish

# Set the following to '0' to disable log messages:
LOGGER ?= 1

# Compiler/linker flags
CFLAGS += -g -Wall -fPIC -DLOGGER=$(LOGGER)
LDLIBS += -lm -lreadline
LDFLAGS += -L. -Wl,-rpath='$$ORIGIN'

src=history.c shell.c ui.c
obj=$(src:.c=.o)

all: $(bin) libshell.so

$(bin): $(obj)
	$(CC) $(CFLAGS) $(LDLIBS) $(LDFLAGS) $(obj) -o $@

libshell.so: $(obj)
	$(CC) $(CFLAGS) $(LDLIBS) $(LDFLAGS) $(obj) -shared -o $@

shell.o: shell.c history.h logger.h ui.h
history.o: history.c history.h logger.h
ui.o: ui.h ui.c logger.h history.h

clean:
	rm -f $(bin) $(obj) libshell.so vgcore.*


# Tests --

test: $(bin) libshell.so ./tests/run_tests
	@DEBUG="$(debug)" ./tests/run_tests $(run)

testupdate: testclean test

./tests/run_tests:
	rm -rf tests
	git clone https://github.com/usf-cs326-fa21/P2-Tests.git tests

testclean:
	rm -rf tests