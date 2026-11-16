#include "codexion.h"

int	sim_is_stopped(t_sim *sim)
{
	int	stopped;

	pthread_mutex_lock(&sim->state_mutex);
	stopped = sim->stop;
	pthread_mutex_unlock(&sim->state_mutex);
	return (stopped);
}