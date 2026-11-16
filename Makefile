NAME = codexion
CC = cc
CFLAGS = -Wall -Wextra -Werror -pthread
CPPFLAGS = -Iincludes

SRCS = src/main.c src/init.c src/cleanup.c src/parse.c src/time.c \
	src/log.c src/monitor.c src/coder.c src/coder.utils.c src/dongle.c \
	src/dongle_wait.c src/heap.c src/heap_utils.c src/sim_utils.c
OBJS = $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re