/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/13 13:14:47 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/17 09:14:13 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

//Includes
# include <pthread.h>
# include <sys/time.h>
# include <unistd.h>
# include <stdlib.h>
# include <stdio.h>
# include <string.h>
# include <limits.h>

typedef struct s_sim	t_sim;

//Struct for the args
typedef struct s_args
{
	int				number_of_coders;
	int				time_to_burnout;
	int				time_to_compile;
	int				time_to_debug;
	int				time_to_refactor;
	int				number_of_compiles_required;
	int				dongle_cooldown;
	char			*schedule;
}	t_args;

//Un node to the heap
typedef struct s_heap_node
{
	long			key;
	int				coder_id;
}	t_heap_node;

//Queue of priority
typedef struct s_heap
{
	t_heap_node		*nodes;
	int				size;
	int				capacity;
}	t_heap;

//Struct for the dongle
typedef struct s_dongle
{
	int				id;
	int				taked;
	long			aviable_at_ms;
	long			arrival_counter;
	t_heap			*waiting;
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
}	t_dongle;

//Struct for the coders
typedef struct s_coder
{
	int				id;
	pthread_t		thread;
	int				thread_launched;
	t_dongle		*left;
	t_dongle		*right;
	long			last_compile_start_ms;
	pthread_mutex_t	mutex;
	int				compilations_done;
	t_sim			*simulation;
}	t_coder;

//Struct of the simulation
struct s_sim
{
	t_args			*args;
	t_dongle		**dongles;
	t_coder			**coders;
	pthread_t		thread;
	int				monitor_launched;
	pthread_mutex_t	mutex_log;
	int				stop_flag;
	pthread_mutex_t	mutex_stop_flag;
	long			start_time_ms;
};

//check_args.c
t_args		*cheack_args(char **argv, int argc);
int			check_struct_values(t_args *args);

//validate_number.c
int			is_valid_number(const char *str);

//heap.c
t_heap		*heap_create(int capacity);
void		heap_destroy(t_heap *heap);
int			heap_push(t_heap *heap, long key, int coder_id);
int			heap_pop_min(t_heap *heap);
int			heap_is_empty(t_heap *heap);

//heap_utils.c 
void		heap_swap(t_heap_node *a, t_heap_node *b);
void		heap_sift_up(t_heap *heap, int i);
void		heap_sift_down(t_heap *heap, int i);

//time_utils.c
long		get_now_ms(void);
void		deadline_in_ms(int ms, struct timespec *out);

//dongle.c
int			sim_is_stopped(t_sim *sim);
t_dongle	*dongle_create(int id, int capacity);
void		dongle_destroy(t_dongle *dongle);
void		dongle_release(t_dongle *dongle, t_coder *coder);

//dongle_acquire.c 
int			dongle_acquire(t_dongle *dongle, t_coder *coder);
int			dongle_acquire_pair(t_coder *coder);

//log.c
long		now_ms(t_sim *sim);
void		log_event(t_sim *sim, int coder_id, const char *event);

//coder_routine.c
void		*coder_routine(void *arg);

//monitor.c
void		*monitor_routine(void *arg);

//sim_builders.c 
t_dongle	**create_dongles(int n);
void		free_dongles(t_dongle **dongles, int count);
t_coder		**create_coders(t_sim *sim);
void		free_coders(t_coder **coders, int count);

//thread_creator.c
t_sim		*build_sim(t_args *args);
int			launch_threads(t_sim *sim);
void		join_threads(t_sim *sim);

#endif
