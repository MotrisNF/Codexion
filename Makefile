NAME	= codexion

CC		= cc
CFLAGS	= -Wall -Wextra -Werror -pthread

SRCS_DIR	= coders

SRCS	=	$(SRCS_DIR)/check_args.c \
			$(SRCS_DIR)/validate_number.c \
			$(SRCS_DIR)/heap.c \
			$(SRCS_DIR)/heap_lookup.c \
			$(SRCS_DIR)/heap_utils.c \
			$(SRCS_DIR)/time_utils.c \
			$(SRCS_DIR)/dongle.c \
			$(SRCS_DIR)/dongle_acquire.c \
			$(SRCS_DIR)/dongle_acquire_fifo.c \
			$(SRCS_DIR)/dongle_acquire_edf.c \
			$(SRCS_DIR)/dongle_topology.c \
			$(SRCS_DIR)/dongle_lock_set.c \
			$(SRCS_DIR)/log.c \
			$(SRCS_DIR)/coder_routine.c \
			$(SRCS_DIR)/monitor.c \
			$(SRCS_DIR)/monitor_utils.c \
			$(SRCS_DIR)/sim_builders.c \
			$(SRCS_DIR)/thread_creator.c \
			$(SRCS_DIR)/main.c

OBJS	= $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

%.o: %.c $(SRCS_DIR)/codexion.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

RACE_CFLAGS	= $(CFLAGS) -fsanitize=thread -g

race: fclean
	$(CC) $(RACE_CFLAGS) $(SRCS) -o $(NAME)

.SILENT: try race_try

EXECUTABLE	= $(shell [ -f ./$(NAME) ] && echo "ok")

try:
ifeq ($(EXECUTABLE),)
	@echo "Error! Run make or make all before doing a test."
else
	@echo "Testing with values 3 300 50 25 10 15 15 edf"
	@sleep 2
	@./$(NAME) 3 300 50 25 10 15 15 edf
	@echo "\nTesting with values 3 300 50 25 10 15 15 fifo"
	@sleep 2
	@./$(NAME) 3 300 50 25 10 15 15 fifo
endif

RUN_NO_ASLR	= setarch $(shell uname -m) -R

race_try: race
ifeq ($(EXECUTABLE),)
	@echo "Error! Run make race before doing a test."
else
	@echo "Testing with ThreadSanitizer values 3 240 50 25 10 15 15 fifo"
	@sleep 2
	@$(RUN_NO_ASLR) ./$(NAME) 3 240 50 25 10 15 15 fifo
	@echo "\nTesting with ThreadSanitizer values 3 240 50 25 10 15 15 edf"
	@sleep 2
	@$(RUN_NO_ASLR) ./$(NAME) 3 240 50 25 10 15 15 edf
endif

.PHONY: all clean fclean re try race race_try
