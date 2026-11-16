#include "codexion.h"

static long	request_key(t_coder *coder, t_sim *sim, long seq)
{
	long	last_compile;

	if (sim->config.scheduler == CODEXION_SCHED_FIFO)
		return (seq);
	pthread_mutex_lock(&coder->lock);
	last_compile = coder->last_compile_ms;
	pthread_mutex_unlock(&coder->lock);
	return (last_compile + sim->config.time_to_burnout);
}

int	dongle_acquire(t_dongle *dongle, t_coder *coder, t_sim *sim)
{
	t_req	req;
	long	seq;
	int		ok;

	pthread_mutex_lock(&dongle->mutex);
	pthread_mutex_lock(&sim->state_mutex);
	if (sim->stop)
	{
		pthread_mutex_unlock(&sim->state_mutex);
		pthread_mutex_unlock(&dongle->mutex);
		return (0);
	}
	seq = sim->next_seq;
	sim->next_seq++;
	pthread_mutex_unlock(&sim->state_mutex);
	req.key = request_key(coder, sim, seq);
	req.seq = seq;
	req.coder_id = coder->id;
	ok = heap_push(&dongle->queue, req);
	if (ok)
	{
		pthread_cond_broadcast(&dongle->cond);
		ok = wait_for_dongle(dongle, coder, sim);
	}
	if (ok)
	{
		heap_pop(&dongle->queue, &req);
		dongle->in_use = 1;
	}
	pthread_mutex_unlock(&dongle->mutex);
	return (ok);
}

void	dongle_release(t_dongle *dongle, t_sim *sim)
{
	pthread_mutex_lock(&dongle->mutex);
	dongle->in_use = 0;
	dongle->available_at_ms = now_ms(sim) + sim->config.dongle_cooldown;
	pthread_cond_broadcast(&dongle->cond);
	pthread_mutex_unlock(&dongle->mutex);
}

void	dongle_wake(t_dongle *dongle)
{
	pthread_mutex_lock(&dongle->mutex);
	pthread_cond_broadcast(&dongle->cond);
	pthread_mutex_unlock(&dongle->mutex);
}