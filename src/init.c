#include "codexion.h"

#include <stdio.h>

int	init_dongles(t_sim *sim)
{
	long		i;
	t_dongle	*dongle;
	int			error;

	if (sim == NULL || sim->dongles == NULL)
		return (0);
	sim->next_seq = 0;
	i = 0;
	while (i < sim->config.number_of_coders)
	{
		dongle = &sim->dongles[i];
		dongle->queue.data = NULL;
		dongle->queue.size = 0;
		dongle->queue.cap = 0;
		dongle->id = i + 1;
		dongle->in_use = 0;
		dongle->available_at_ms = 0;
		error = pthread_mutex_init(&dongle->mutex, NULL);
		if (error != 0)
		{
			fprintf(stderr, "Error: cannot initialize dongle mutex.\n");
			return (0);
		}
		sim->initialized_dongle_mutexes++;
		error = pthread_cond_init(&dongle->cond, NULL);
		if (error != 0)
		{
			fprintf(stderr, "Error: cannot initialize dongle condition.\n");
			return (0);
		}
		sim->initialized_dongle_conds++;
		if (!heap_init(&dongle->queue, 2))
		{
			fprintf(stderr, "Error: cannot initialize dongle queue.\n");
			return (0);
		}
		sim->initialized_dongle_heaps++;
		i++;
	}
	return (1);
}

static void	set_dongles(t_coder *coder, t_sim *sim, long i)
{
	long	next;

	next = (i + 1) % sim->config.number_of_coders;
	if (i < next)
	{
		coder->first_dongle = &sim->dongles[i];
		coder->second_dongle = &sim->dongles[next];
	}
	else
	{
		coder->first_dongle = &sim->dongles[next];
		coder->second_dongle = &sim->dongles[i];
	}
}

int	init_coders(t_sim *sim)
{
	long	i;
	int		error;

	if (sim == NULL || sim->coders == NULL || sim->dongles == NULL)
		return (0);
	i = 0;
	while (i < sim->config.number_of_coders)
	{
		sim->coders[i].id = i + 1;
		sim->coders[i].sim = sim;
		sim->coders[i].last_compile_ms = 0;
		sim->coders[i].compiles_done = 0;
		sim->coders[i].thread_started = 0;
		set_dongles(&sim->coders[i], sim, i);
		error = pthread_mutex_init(&sim->coders[i].lock, NULL);
		if (error != 0)
		{
			fprintf(stderr, "Error: cannot initialize coder mutex.\n");
			return (0);
		}
		sim->initialized_coders++;
		i++;
	}
	return (1);
}