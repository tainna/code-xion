#include <time.h>
#include "codexion.h"

static long	clock_ms(void)
{
	struct timespec	ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
		return (-1);
	return ((long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

int	init_start_time(t_sim *sim)
{
	if (sim == NULL)
		return (-1);
	sim->start_ms = clock_ms();
	if (sim->start_ms < 0)
		return (-1);
	return (0);
}

long	now_ms(const t_sim *sim)
{
	long	now;

	now = clock_ms();
	if (sim == NULL || now < 0)
		return (-1);
	return (now - sim->start_ms);
}