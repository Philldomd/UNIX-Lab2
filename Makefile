CC = g++
LD = g++
CFLAGS = -g -Wall
LDFLAGS = 
RM = /bin/rm -f
SRCSD = source
OUTD = build
SRCS = $(wildcard $(SRCSD)/*.cpp)
OBJS = $(patsubst $(SRCSD)/%.cpp,$(OUTD)/%.o,$(SRCS))
PROG = ServerSebPhi

.PHONY: directories

all: directories $(PROG)

directories: $(OUTD)

$(OUTD):
	mkdir -p $(OUTD)

$(PROG): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $(PROG)

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
