#include "codexion.h"

#include <stdio.h>
#include <stdlib.h>

void	cleanup(t_sim *sim)
{
	long	i;
	int		error;

	if (sim == NULL)
		return ;
	i = sim->initialized_coders;
	while (i > 0)
	{
		i--;
		error = pthread_mutex_destroy(&sim->coders[i].lock);
		if (error != 0)
			fprintf(stderr, "Error: cannot destroy coder mutex.\n");
	}
	i = sim->initialized_dongle_heaps;
	while (i > 0)
	{
		i--;
		heap_free(&sim->dongles[i].queue);
	}
	i = sim->initialized_dongle_conds;
	while (i > 0)
	{
		i--;
		error = pthread_cond_destroy(&sim->dongles[i].cond);
		if (error != 0)
			fprintf(stderr, "Error: cannot destroy dongle condition.\n");
	}
	i = sim->initialized_dongle_mutexes;
	while (i > 0)
	{
		i--;
		error = pthread_mutex_destroy(&sim->dongles[i].mutex);
		if (error != 0)
			fprintf(stderr, "Error: cannot destroy dongle mutex.\n");
	}
	if (sim->state_mutex_initialized)
	{
		error = pthread_mutex_destroy(&sim->state_mutex);
		if (error != 0)
			fprintf(stderr, "Error: cannot destroy state mutex.\n");
	}
	if (sim->log_mutex_initialized)
	{
		error = pthread_mutex_destroy(&sim->log_mutex);
		if (error != 0)
			fprintf(stderr, "Error: cannot destroy log mutex.\n");
	}
	free(sim->coders);
	free(sim->dongles);
	sim->coders = NULL;
	sim->dongles = NULL;
}