TARGETS = scheduling_simulator
CC = gcc
CFLAGS += -std=gnu99 -Wall
OBJS = scheduling_simulator.o task.o

GIT_HOOKS := .git/hooks/applied
all:$(GIT_HOOKS) $(TARGETS)

$(GIT_HOOKS):
	@.githooks/install-git-hooks
	@echo
$(TARGETS):$(OBJS)
	$(CC) $(CFLAGS) -o scheduling_simulator $(OBJS)

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -g -c $< -o $@

clean:
	rm -rf *.o scheduling_simulator

AS:
	astyle --style=kr --indent=spaces=4 task.c
	astyle --style=kr --indent=spaces=4 scheduling_simulator.c
debug:
	$(CC) $(CFLAGS) -o scheduling_simulator $(OBJS) -g scheduling_simulator.c

