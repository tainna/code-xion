#include "codexion.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void	wake_all_dongles(t_sim *sim)
{
	long	i;

	i = 0;
	while (i < sim->config.number_of_coders)
	{
		dongle_wake(&sim->dongles[i]);
		i++;
	}
}

static int	stop_and_wake(t_sim *sim)
{
	int	error;

	error = pthread_mutex_lock(&sim->log_mutex);
	if (error != 0)
	{
		fprintf(stderr, "Error: cannot lock log mutex while stopping: %s\n",
			strerror(error));
		return (0);
	}
	error = pthread_mutex_lock(&sim->state_mutex);
	if (error != 0)
	{
		pthread_mutex_unlock(&sim->log_mutex);
		fprintf(stderr, "Error: cannot lock state mutex while stopping: %s\n",
			strerror(error));
		return (0);
	}
	sim->stop = 1;
	error = pthread_mutex_unlock(&sim->state_mutex);
	if (error != 0)
	{
		pthread_mutex_unlock(&sim->log_mutex);
		fprintf(stderr, "Error: cannot unlock state mutex while stopping: %s\n",
			strerror(error));
		return (0);
	}
	error = pthread_mutex_unlock(&sim->log_mutex);
	if (error != 0)
	{
		fprintf(stderr, "Error: cannot unlock log mutex while stopping: %s\n",
			strerror(error));
		return (0);
	}
	wake_all_dongles(sim);
	return (1);
}

static int	join_coders(t_sim *sim)
{
	long	i;
	int		error;
	int		ok;

	i = 0;
	ok = 1;
	while (i < sim->config.number_of_coders)
	{
		if (sim->coders[i].thread_started)
		{
			error = pthread_join(sim->coders[i].thread, NULL);
			if (error != 0)
			{
				fprintf(stderr, "Error: cannot join coder thread: %s\n",
					strerror(error));
				ok = 0;
			}
			else
				sim->coders[i].thread_started = 0;
		}
		i++;
	}
	return (ok);
}

static int	start_coders(t_sim *sim)
{
	long	i;
	int		error;

	i = 0;
	while (i < sim->config.number_of_coders)
	{
		error = pthread_create(&sim->coders[i].thread, NULL, coder_routine,
				&sim->coders[i]);
		if (error != 0)
		{
			fprintf(stderr, "Error: cannot create coder thread: %s\n",
				strerror(error));
			return (0);
		}
		sim->coders[i].thread_started = 1;
		i++;
	}
	return (1);
}

static int	initialize_sim(t_sim *sim, int argc, char **argv)
{
	size_t	count;
	int		error;

	if (!parse_args(argc, argv, &sim->config))
		return (0);
	count = (size_t)sim->config.number_of_coders;
	if (count > SIZE_MAX / sizeof(*sim->coders)
		|| count > SIZE_MAX / sizeof(*sim->dongles))
	{
		fprintf(stderr, "Error: too many coders.\n");
		return (0);
	}
	sim->coders = malloc(count * sizeof(*sim->coders));
	sim->dongles = malloc(count * sizeof(*sim->dongles));
	if (sim->coders == NULL || sim->dongles == NULL)
	{
		fprintf(stderr, "Error: cannot allocate simulation arrays.\n");
		return (0);
	}
	memset(sim->coders, 0, count * sizeof(*sim->coders));
	memset(sim->dongles, 0, count * sizeof(*sim->dongles));
	error = pthread_mutex_init(&sim->log_mutex, NULL);
	if (error != 0)
	{
		fprintf(stderr, "Error: cannot initialize log mutex.\n");
		return (0);
	}
	sim->log_mutex_initialized = 1;
	error = pthread_mutex_init(&sim->state_mutex, NULL);
	if (error != 0)
	{
		fprintf(stderr, "Error: cannot initialize state mutex.\n");
		return (0);
	}
	sim->state_mutex_initialized = 1;
	sim->stop = 0;
	if (init_start_time(sim) != 0)
	{
		fprintf(stderr, "Error: cannot initialize simulation clock.\n");
		return (0);
	}
	if (!init_dongles(sim) || !init_coders(sim))
		return (0);
	return (1);
}

int	main(int argc, char **argv)
{
	t_sim	sim;
	int		error;

	memset(&sim, 0, sizeof(sim));
	if (!initialize_sim(&sim, argc, argv))
	{
		cleanup(&sim);
		return (EXIT_FAILURE);
	}
	if (!start_coders(&sim))
	{
		if (!stop_and_wake(&sim))
			return (EXIT_FAILURE);
		if (!join_coders(&sim))
			return (EXIT_FAILURE);
		cleanup(&sim);
		return (EXIT_FAILURE);
	}
	error = pthread_create(&sim.monitor, NULL, monitor_routine, &sim);
	if (error != 0)
	{
		fprintf(stderr, "Error: cannot create monitor thread: %s\n",
			strerror(error));
		if (!stop_and_wake(&sim))
			return (EXIT_FAILURE);
		if (!join_coders(&sim))
			return (EXIT_FAILURE);
		cleanup(&sim);
		return (EXIT_FAILURE);
	}
	error = pthread_join(sim.monitor, NULL);
	if (error != 0)
	{
		fprintf(stderr, "Error: cannot join monitor thread: %s\n",
			strerror(error));
		stop_and_wake(&sim);
		join_coders(&sim);
		return (EXIT_FAILURE);
	}
	if (!join_coders(&sim))
		return (EXIT_FAILURE);
	cleanup(&sim);
	return (EXIT_SUCCESS);
}