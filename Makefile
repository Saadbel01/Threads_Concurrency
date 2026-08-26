NAME = codexion
CC = gcc
CFLAGS = -Wall -Wextra -Werror -pthread
SRCS = main.c parser.c init.c coder.c monitor.c dongle.c dongle_utils.c heap.c helpers.c helpers2.c
OBJS = $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

%.o: %.c codexion.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re