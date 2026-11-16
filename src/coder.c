#include "codexion.h"

#include <stdio.h>
#include <unistd.h>

static int	sleep_ms(t_sim *sim, long duration)
{
	while (duration > 0 && !sim_is_stopped(sim))
	{
		if (usleep(1000) != 0)
		{
			perror("Error: usleep");
			return (0);
		}
		duration--;
	}
	return (!sim_is_stopped(sim));
}

static int	increment_compile_count(t_coder *coder)
{
	int	error;

	error = pthread_mutex_lock(&coder->lock);
	if (error != 0)
	{
		fprintf(stderr, "Error: cannot lock coder state (%d).\n", error);
		return (0);
	}
	coder->compiles_done++;
	error = pthread_mutex_unlock(&coder->lock);
	if (error != 0)
	{
		fprintf(stderr, "Error: cannot unlock coder state (%d).\n", error);
		return (0);
	}
	return (1);
}

static long	compile_count(t_coder *coder)
{
	long	count;
	int		error;

	error = pthread_mutex_lock(&coder->lock);
	if (error != 0)
	{
		fprintf(stderr, "Error: cannot lock coder state (%d).\n", error);
		return (-1);
	}
	count = coder->compiles_done;
	error = pthread_mutex_unlock(&coder->lock);
	if (error != 0)
	{
		fprintf(stderr, "Error: cannot unlock coder state (%d).\n", error);
		return (-1);
	}
	return (count);
}

static int	mark_compile_start(t_coder *coder)
{
	long	now;
	int		error;

	now = now_ms(coder->sim);
	if (now < 0)
	{
		fprintf(stderr, "Error: cannot read simulation clock.\n");
		return (0);
	}
	error = pthread_mutex_lock(&coder->lock);
	if (error != 0)
	{
		fprintf(stderr, "Error: cannot lock coder state (%d).\n", error);
		return (0);
	}
	coder->last_compile_ms = now;
	error = pthread_mutex_unlock(&coder->lock);
	if (error != 0)
	{
		fprintf(stderr, "Error: cannot unlock coder state (%d).\n", error);
		return (0);
	}
	return (1);
}

static void	release_pair(t_coder *coder)
{
	dongle_release(coder->second_dongle, coder->sim);
	dongle_release(coder->first_dongle, coder->sim);
}

static int	acquire_pair(t_coder *coder)
{
	if (!dongle_acquire(coder->first_dongle, coder, coder->sim))
		return (0);
	if (!log_state(coder->sim, coder->id, "has taken a dongle"))
	{
		dongle_release(coder->first_dongle, coder->sim);
		return (0);
	}
	if (!dongle_acquire(coder->second_dongle, coder, coder->sim))
	{
		dongle_release(coder->first_dongle, coder->sim);
		return (0);
	}
	if (!log_state(coder->sim, coder->id, "has taken a dongle"))
	{
		release_pair(coder);
		return (0);
	}
	return (1);
}

static int	run_compile_cycle(t_coder *coder)
{
	t_sim	*sim;

	sim = coder->sim;
	if (!acquire_pair(coder))
		return (0);
	if (!mark_compile_start(coder)
		|| !log_state(sim, coder->id, "is compiling"))
	{
		release_pair(coder);
		return (0);
	}
	if (!sleep_ms(sim, sim->config.time_to_compile))
	{
		release_pair(coder);
		return (0);
	}
	release_pair(coder);
	if (!increment_compile_count(coder))
		return (0);
	if (compile_count(coder) >= sim->config.compiles_required)
		return (1);
	if (!log_state(sim, coder->id, "is debugging")
		|| !sleep_ms(sim, sim->config.time_to_debug)
		|| !log_state(sim, coder->id, "is refactoring")
		|| !sleep_ms(sim, sim->config.time_to_refactor))
		return (0);
	return (1);
}

int	coder_assign_dongles(t_sim *sim)
{
	long	count;
	long	index;
	long	next;
	long	first;
	long	second;

	if (sim == NULL || sim->coders == NULL || sim->dongles == NULL)
		return (0);
	count = sim->config.number_of_coders;
	if (count < 1)
		return (0);
	index = 0;
	while (index < count)
	{
		next = (index + 1) % count;
		first = index;
		second = next;
		if (next < index)
		{
			first = next;
			second = index;
		}
		sim->coders[index].id = index + 1;
		sim->coders[index].sim = sim;
		sim->coders[index].first_dongle = &sim->dongles[first];
		sim->coders[index].second_dongle = &sim->dongles[second];
		sim->dongles[index].id = index + 1;
		index++;
	}
	return (1);
}

static void	wait_for_monitor(t_sim *sim)
{
	while (!sim_is_stopped(sim))
		usleep(1000);
}

void	*coder_routine(void *arg)
{
	t_coder	*coder;
	long	count;

	coder = (t_coder *)arg;
	if (coder == NULL || coder->sim == NULL)
		return (NULL);
	if (coder->first_dongle == coder->second_dongle)
	{
		wait_for_monitor(coder->sim);
		return (NULL);
	}
	while (!sim_is_stopped(coder->sim))
	{
		count = compile_count(coder);
		if (count < 0 || count >= coder->sim->config.compiles_required)
			break ;
		if (!run_compile_cycle(coder))
			break ;
	}
	return (NULL);
}