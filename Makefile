NAME = test

all: $(NAME)

$(NAME): test.c
	gcc test.c -lpthread -o $(NAME)

clean:
	rm -f $(NAME)

re: clean all