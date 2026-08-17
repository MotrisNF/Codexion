NAME	= codexion

CC		= cc
CFLAGS	= -Wall -Wextra -Werror -pthread

SRCS	=	check_args.c \
			validate_number.c \
			heap.c \
			heap_utils.c \
			time_utils.c \
			dongle.c \
			dongle_acquire.c \
			log.c \
			coder_routine.c \
			monitor.c \
			sim_builders.c \
			thread_creator.c \
			main.c

OBJS	= $(SRCS:.c=.o)

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
