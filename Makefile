
CC = gcc

CFLAGS = -I ./NtyCo/core/ -g -O0 -fno-omit-frame-pointer
LDFLAGS = -L ./NtyCo/
LDLIBS = -lntyco

SRCS = main.c kvstore.c ntyco_entry.c epoll_entry.c kvstore_array.c kvstore_rbtree.c
TESTCASE_SRCS = testcase.c

TARGET = kvstore
SUBDIR = ./NtyCo/
TESTCASE = testcase

OBJS = $(SRCS:.c=.o)

all: $(SUBDIR) $(TARGET) $(TESTCASE)

$(SUBDIR): ECHO
	make -C $@

ECHO:
	@echo $(SUBDIR)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(TESTCASE): $(TESTCASE_SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS) $(TARGET) $(TESTCASE)
