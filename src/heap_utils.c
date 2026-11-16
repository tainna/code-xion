#include "codexion.h"

int	heap_less(t_req a, t_req b)
{
	if (a.key != b.key)
		return (a.key < b.key);
	if (a.seq != b.seq)
		return (a.seq < b.seq);
	return (a.coder_id < b.coder_id);
}

static void	heap_swap(t_req *a, t_req *b)
{
	t_req	tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}

void	heap_sift_up(t_heap *h, int index)
{
	int	parent;

	while (index > 0)
	{
		parent = (index - 1) / 2;
		if (!heap_less(h->data[index], h->data[parent]))
			break ;
		heap_swap(&h->data[index], &h->data[parent]);
		index = parent;
	}
}

void	heap_sift_down(t_heap *h, int index)
{
	int	left;
	int	right;
	int	smallest;

	while (index < h->size / 2)
	{
		left = index * 2 + 1;
		right = left + 1;
		smallest = left;
		if (right < h->size && heap_less(h->data[right], h->data[left]))
			smallest = right;
		if (!heap_less(h->data[smallest], h->data[index]))
			break ;
		heap_swap(&h->data[index], &h->data[smallest]);
		index = smallest;
	}
}