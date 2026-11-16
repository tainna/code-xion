#include <unistd.h>
#include "codexion.h"

int	sleep_ms(t_sim *sim, long duration)
{
	long	end;
	long	remaining;

	end = now_ms(sim) + duration;
	while (!sim_is_stopped(sim))
	{
		remaining = end - now_ms(sim);
		if (remaining <= 0)
			return (1);
		if (remaining > 1)
			remaining = 1;
		usleep(remaining * 1000);
	}
	return (0);
}

void	mark_compile_start(t_coder *coder)
{
	long	now;

	now = now_ms(coder->sim);
	pthread_mutex_lock(&coder->lock);
	coder->last_compile_ms = now;
	pthread_mutex_unlock(&coder->lock);
}

void	count_compile(t_coder *coder)
{
	pthread_mutex_lock(&coder->lock);
	coder->compiles_done++;
	pthread_mutex_unlock(&coder->lock);
}