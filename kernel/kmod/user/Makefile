CC =gcc
BIN	=app

SRCS = $(wildcard ./*.c)


C_INCLUDES =  \
-I../include \
-I../include/user \

# OBJS =$(SRCS:.c=.o)

all: $(BIN)

$(BIN): 
	$(CC) $(SRCS) $(C_INCLUDES)  -o $(BIN) $(OBJS) -g

clean:
	rm -f $(BIN) $(OBJS)