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

$(OUTD): $(OUTD)
	mkdir -p $(OUTD)

$(PROG): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $(PROG)

$(OUTD)/%.o: $(SRCSD)/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(PROG)
	$(RM) -R $(OUTD)
