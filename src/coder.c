#include <unistd.h>
#include "codexion.h"

static void	release_pair(t_coder *coder)
{
	dongle_release(coder->second_dongle, coder->sim);
	dongle_release(coder->first_dongle, coder->sim);
}

static int	acquire_pair(t_coder *coder)
{
	t_sim	*sim;

	sim = coder->sim;
	if (!dongle_acquire(coder->first_dongle, coder, sim))
		return (0);
	log_state(sim, coder->id, "has taken a dongle");
	if (!dongle_acquire(coder->second_dongle, coder, sim))
	{
		dongle_release(coder->first_dongle, sim);
		return (0);
	}
	log_state(sim, coder->id, "has taken a dongle");
	return (1);
}

static int	run_compile_cycle(t_coder *coder)
{
	t_sim	*sim;

	sim = coder->sim;
	if (!acquire_pair(coder))
		return (0);
	mark_compile_start(coder);
	if (!log_state(sim, coder->id, "is compiling")
		|| !sleep_ms(sim, sim->config.time_to_compile))
	{
		release_pair(coder);
		return (0);
	}
	release_pair(coder);
	count_compile(coder);
	if (!log_state(sim, coder->id, "is debugging")
		|| !sleep_ms(sim, sim->config.time_to_debug)
		|| !log_state(sim, coder->id, "is refactoring")
		|| !sleep_ms(sim, sim->config.time_to_refactor))
		return (0);
	return (1);
}

static void	single_coder(t_coder *coder)
{
	t_sim	*sim;

	sim = coder->sim;
	if (!dongle_acquire(coder->first_dongle, coder, sim))
		return ;
	log_state(sim, coder->id, "has taken a dongle");
	while (!sim_is_stopped(sim))
		usleep(1000);
	dongle_release(coder->first_dongle, sim);
}

void	*coder_routine(void *arg)
{
	t_coder	*coder;

	coder = (t_coder *)arg;
	if (coder->first_dongle == coder->second_dongle)
		single_coder(coder);
	else
	{
		while (!sim_is_stopped(coder->sim))
		{
			if (!run_compile_cycle(coder))
				break ;
		}
	}
	return (NULL);
}