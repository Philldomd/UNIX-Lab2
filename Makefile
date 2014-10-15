CC = g++
LD = g++
CFLAGS = -g -Wall -std=c++0x

LDFLAGS =
LIBS = -lmagic -lpthread
RM = /bin/rm -f
SRCSD = source
OUTD = build
SRCS = $(wildcard $(SRCSD)/*.cpp)
OBJS = $(patsubst $(SRCSD)/%.cpp,$(OUTD)/%.o,$(SRCS))
PROG = ServerSebPhi

ifdef ThreadPool
CFLAGS += -DIO_MULT_THREAD_POOL
PROG := $(PROG)IOTP
endif

ifdef Fork
CFLAGS += -DFORK
PROG := $(PROG)Fork
endif

.PHONY: directories

all: directories $(PROG)

directories: $(OUTD)

$(OUTD):
	mkdir -p $(OUTD)

$(PROG): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $(PROG)

$(OUTD)/depend: directories $(OUTD)/.depend

$(OUTD)/.depend: $(SRCS)
	rm -f $(OUTD)/.depend
	$(CC) $(CFLAGS) -MM $^ | sed "s/^.*\.o/${OUTD}\/&/" > $(OUTD)/.depend;
	
-include $(OUTD)/.depend

$(OUTD)/%.o: $(SRCSD)/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(PROG)
	$(RM) -R $(OUTD)
