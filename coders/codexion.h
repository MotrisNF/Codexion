/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/13 13:14:47 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/16 00:00:00 by saperez-         ###   ########.fr       */
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

//Declaracion adelantada: t_coder necesita un puntero a t_sim y t_sim
//necesita un array de t_coder, asi que uno de los dos tiene que
//"prometerse" antes de existir del todo. Su cuerpo se completa al final.
typedef struct s_sim	t_sim;

//Struct para guardar los argumentos recibidos por el programa
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

//Un nodo del heap: la clave por la que se ordena (orden de llegada en
//fifo, deadline en edf) y a que coder pertenece.
typedef struct s_heap_node
{
	long			key;
	int				coder_id;
}	t_heap_node;

//Cola de prioridad (min-heap binario) sobre un array de tamano fijo.
//Se usa una por cada dongle, para decidir a quien se le da ese dongle
//cuando varios coders lo estan esperando a la vez.
typedef struct s_heap
{
	t_heap_node		*nodes;
	int				size;
	int				capacity;
}	t_heap;

//Struct para el dongle. "waiting" es el heap de coders que estan
//esperando este dongle en concreto (arbitraje por dongle, no global).
//"arrival_counter" solo se usa para la clave en modo fifo.
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

//Struct para los coders. "left"/"right" son los dongles que le tocan
//segun su posicion en la mesa circular (si number_of_coders == 1,
//left y right apuntan al mismo dongle).
typedef struct s_coder
{
	int				id;
	pthread_t		thread;
	t_dongle		*left;
	t_dongle		*right;
	long			last_compile_start_ms;
	pthread_mutex_t	mutex;
	int				compilations_done;
	t_sim			*simulation;
}	t_coder;

//Struct raiz de la simulacion (creado una unica vez en main, y pasado
//por puntero a todos los hilos: asi no hacen falta variables globales).
struct s_sim
{
	t_args			*args;
	t_dongle		**dongles;
	t_coder			**coders;
	pthread_t		thread;
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

//heap_utils.c (reordenamiento interno del heap, separado de heap.c
//por limite de funciones por archivo de la Norma)
void		heap_swap(t_heap_node *a, t_heap_node *b);
void		heap_sift_up(t_heap *heap, int i);
void		heap_sift_down(t_heap *heap, int i);

//dongle.c
long		get_now_ms(void);
int			sim_is_stopped(t_sim *sim);
t_dongle	*dongle_create(int id, int capacity);
void		dongle_destroy(t_dongle *dongle);
void		dongle_release(t_dongle *dongle, t_coder *coder);

//dongle_acquire.c (separado de dongle.c por limite de funciones
//por archivo de la Norma)
int			dongle_acquire(t_dongle *dongle, t_coder *coder);
int			dongle_acquire_pair(t_coder *coder);

//log.c (pendiente: punto 5 de la guia, dongle.c ya depende de ella)
void		log_event(t_sim *sim, int coder_id, const char *event);

#endif
