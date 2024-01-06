NAME        := writer

SRCS        := $(NAME).c
OBJS        := $(NAME).o


RM          := rm -f
MAKEFLAGS   += --no-print-directory

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CCFLAGS) $(LDFLAGS) $(INCLUDES) $(OBJS) -o $(NAME)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	$(RM) $(OBJS) $(NAME)

re:
	$(MAKE) fclean
	$(MAKE) all

.PHONY: clean re
