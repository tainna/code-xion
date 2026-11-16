#include <stdlib.h>
#include "codexion.h"

int	heap_init(t_heap *h, int cap)
{
	if (h == NULL || cap <= 0)
		return (0);
	h->size = 0;
	h->cap = 0;
	h->data = malloc(sizeof(t_req) * cap);
	if (h->data == NULL)
		return (0);
	h->cap = cap;
	return (1);
}

int	heap_push(t_heap *h, t_req value)
{
	if (h == NULL || h->data == NULL || h->size >= h->cap)
		return (0);
	h->data[h->size] = value;
	h->size++;
	heap_sift_up(h, h->size - 1);
	return (1);
}

int	heap_pop(t_heap *h, t_req *out)
{
	if (h == NULL || out == NULL || h->data == NULL || h->size <= 0)
		return (0);
	*out = h->data[0];
	h->size--;
	if (h->size > 0)
	{
		h->data[0] = h->data[h->size];
		heap_sift_down(h, 0);
	}
	return (1);
}

int	heap_peek(const t_heap *h, t_req *out)
{
	if (h == NULL || out == NULL || h->data == NULL || h->size <= 0)
		return (0);
	*out = h->data[0];
	return (1);
}

void	heap_free(t_heap *h)
{
	if (h == NULL)
		return ;
	free(h->data);
	h->data = NULL;
	h->size = 0;
	h->cap = 0;
}