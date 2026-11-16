#include "codexion.h"

#include <stdio.h>
#include <string.h>

int	log_state(t_sim *sim, long id, const char *state)
{
	long	now;
	int		stopped;
	int		is_burnout;
	int		error;

	if (sim == NULL || state == NULL)
		return (0);
	is_burnout = (strcmp(state, "burned out") == 0);
	error = pthread_mutex_lock(&sim->log_mutex);
	if (error != 0)
		return (0);
	error = pthread_mutex_lock(&sim->state_mutex);
	if (error != 0)
	{
		pthread_mutex_unlock(&sim->log_mutex);
		return (0);
	}
	stopped = sim->stop;
	if (!stopped || is_burnout)
	{
		now = now_ms(sim);
		if (now < 0)
		{
			pthread_mutex_unlock(&sim->state_mutex);
			pthread_mutex_unlock(&sim->log_mutex);
			fprintf(stderr, "Error: cannot read simulation clock.\n");
			return (0);
		}
		printf("%ld %ld %s\n", now, id, state);
		fflush(stdout);
	}
	error = pthread_mutex_unlock(&sim->state_mutex);
	if (error != 0)
	{
		pthread_mutex_unlock(&sim->log_mutex);
		return (0);
	}
	error = pthread_mutex_unlock(&sim->log_mutex);
	return (error == 0);
}