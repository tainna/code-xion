#include <stdio.h>
#include "codexion.h"

int	log_state(t_sim *sim, long id, const char *state)
{
	pthread_mutex_lock(&sim->log_mutex);
	if (sim_is_stopped(sim))
	{
		pthread_mutex_unlock(&sim->log_mutex);
		return (0);
	}
	printf("%ld %ld %s\n", now_ms(sim), id, state);
	pthread_mutex_unlock(&sim->log_mutex);
	return (1);
}