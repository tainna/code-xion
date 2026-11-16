#include <time.h>
#include "codexion.h"

static void	make_deadline(long wait_ms, struct timespec *ts)
{
	clock_gettime(CLOCK_REALTIME, ts);
	ts->tv_sec += wait_ms / 1000;
	ts->tv_nsec += (wait_ms % 1000) * 1000000L;
	if (ts->tv_nsec >= 1000000000L)
	{
		ts->tv_sec++;
		ts->tv_nsec -= 1000000000L;
	}
}

static int	is_my_turn(t_dongle *dongle, long coder_id)
{
	t_req	head;

	if (!heap_peek(&dongle->queue, &head))
		return (0);
	return (head.coder_id == coder_id && !dongle->in_use);
}

static void	timed_wait(t_dongle *dongle, long wait_ms)
{
	struct timespec	ts;

	make_deadline(wait_ms, &ts);
	pthread_cond_timedwait(&dongle->cond, &dongle->mutex, &ts);
}

int	wait_for_dongle(t_dongle *dongle, t_coder *coder, t_sim *sim)
{
	long	now;

	while (!sim_is_stopped(sim))
	{
		if (!is_my_turn(dongle, coder->id))
			pthread_cond_wait(&dongle->cond, &dongle->mutex);
		else
		{
			now = now_ms(sim);
			if (now >= dongle->available_at_ms)
				return (1);
			timed_wait(dongle, dongle->available_at_ms - now);
		}
	}
	return (0);
}