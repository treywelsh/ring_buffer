CFLAGS ?= -Wall -Wextra
LDFLAGS ?=
CC ?= gcc
INCLUDE= -I ../../include/

BIN = ringb_test
SRC = main.c
SRC_THREAD = main_thread.c
OBJ = $(SRC:.c=.o)
OBJ_THREAD = $(SRC_THREAD:.c=.o)

#Compile options
DEBUG ?= 0
STATIC ?= 0
THREAD ?= 0
SANITIZE ?= 0

#Mandatory flags
CFLAGS += -fstrict-aliasing ${INCLUDE}

ifeq ($(DEBUG), 1)
	CFLAGS += -O0 -DDEBUG -g -fno-inline
else
	CFLAGS += -O3 -DNDEBUG -march=native
endif

ifeq ($(SANITIZE), 1)
	CFLAGS += -fsanitize=thread
endif

ifeq ($(STATIC), 1)
	LDFLAGS += -static
endif

ifeq ($(THREAD), 1)
	SRC = ${SRC_THREAD}
	OBJ = $(OBJ_THREAD)
	CFLAGS += -std=c11 -D RINGB_SPSC_SAFE -D _BSD_SOURCE #_BSD_SOURCE:  usleep
	LDFLAGS += -lpthread
endif

all: $(BIN)

$(BIN) : $(OBJ)
	$(CC) -o $(BIN) $(OBJ) $(LDFLAGS) $(CFLAGS)

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean :
	@rm -f $(OBJ) $(OBJ_THREAD) $(BIN)
