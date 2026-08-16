/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/16 00:00:00 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

//Min-heap binario sobre un array de tamano fijo: el nodo con la
//"key" mas pequena esta siempre en la posicion 0. No es thread-safe
//por si mismo: quien lo use (dongle.c) tiene que proteger cada
//heap_push/heap_pop_min con el mutex del dongle al que pertenece.
//El reordenamiento (subir/bajar) vive en heap_utils.c, por limite
//de funciones por archivo de la Norma.

t_heap	*heap_create(int capacity)
{
	t_heap	*heap;

	heap = malloc(sizeof(t_heap));
	if (!heap)
		return (NULL);
	heap->nodes = malloc(sizeof(t_heap_node) * capacity);
	if (!heap->nodes)
		return (free(heap), NULL);
	heap->size = 0;
	heap->capacity = capacity;
	return (heap);
}

void	heap_destroy(t_heap *heap)
{
	if (!heap)
		return ;
	free(heap->nodes);
	free(heap);
}

int	heap_is_empty(t_heap *heap)
{
	return (heap->size == 0);
}

int	heap_push(t_heap *heap, long key, int coder_id)
{
	if (heap->size >= heap->capacity)
		return (0);
	heap->nodes[heap->size].key = key;
	heap->nodes[heap->size].coder_id = coder_id;
	heap_sift_up(heap, heap->size);
	heap->size++;
	return (1);
}

//Extrae y devuelve el coder_id de la key minima (la raiz), moviendo
//el ultimo elemento a la raiz y reordenando hacia abajo.
int	heap_pop_min(t_heap *heap)
{
	int	coder_id;

	if (heap->size == 0)
		return (-1);
	coder_id = heap->nodes[0].coder_id;
	heap->size--;
	heap->nodes[0] = heap->nodes[heap->size];
	heap_sift_down(heap, 0);
	return (coder_id);
}
