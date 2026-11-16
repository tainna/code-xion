#include <stdio.h>
#include <unistd.h>
#include "codexion.h"

static void	wake_dongles(t_sim *sim)
{
	long	i;

	i = 0;
	while (i < sim->config.number_of_coders)
	{
		dongle_wake(&sim->dongles[i]);
		i++;
	}
}

static void	stop_simulation(t_sim *sim, long burnout_id)
{
	pthread_mutex_lock(&sim->log_mutex);
	pthread_mutex_lock(&sim->state_mutex);
	sim->stop = 1;
	pthread_mutex_unlock(&sim->state_mutex);
	if (burnout_id != 0)
	{
		printf("%ld %ld burned out\n", now_ms(sim), burnout_id);
		fflush(stdout);
	}
	pthread_mutex_unlock(&sim->log_mutex);
	wake_dongles(sim);
}

static void	read_coder(t_coder *coder, long *last, long *done)
{
	pthread_mutex_lock(&coder->lock);
	*last = coder->last_compile_ms;
	*done = coder->compiles_done;
	pthread_mutex_unlock(&coder->lock);
}

static int	check_coders(t_sim *sim, long now, long *burnout_id)
{
	long	i;
	long	last;
	long	done;
	int		all_done;

	i = 0;
	all_done = 1;
	*burnout_id = 0;
	while (i < sim->config.number_of_coders)
	{
		read_coder(&sim->coders[i], &last, &done);
		if (done < sim->config.compiles_required)
			all_done = 0;
		if (*burnout_id == 0 && now - last > sim->config.time_to_burnout)
			*burnout_id = sim->coders[i].id;
		i++;
	}
	return (all_done);
}

void	*monitor_routine(void *arg)
{
	t_sim	*sim;
	long	burnout_id;
	int		all_done;

	sim = (t_sim *)arg;
	while (1)
	{
		usleep(1000);
		all_done = check_coders(sim, now_ms(sim), &burnout_id);
		if (all_done)
			burnout_id = 0;
		if (all_done || burnout_id != 0)
		{
			stop_simulation(sim, burnout_id);
			return (NULL);
		}
	}
}