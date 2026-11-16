#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>

typedef enum e_scheduler
{
	CODEXION_SCHED_FIFO,
	CODEXION_SCHED_EDF
}	t_scheduler;

typedef struct s_config
{
	long		number_of_coders;
	long		time_to_burnout;
	long		time_to_compile;
	long		time_to_debug;
	long		time_to_refactor;
	long		compiles_required;
	long		dongle_cooldown;
	t_scheduler	scheduler;
}	t_config;

typedef struct s_sim	t_sim;

typedef struct s_req
{
	long	key;
	long	seq;
	long	coder_id;
}	t_req;

typedef struct s_heap
{
	t_req	*data;
	int		size;
	int		cap;
}	t_heap;

typedef struct s_dongle
{
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
	t_heap			queue;
	long			id;
	int				in_use;
	long			available_at_ms;
}	t_dongle;

typedef struct s_coder
{
	long			id;
	long			last_compile_ms;
	long			compiles_done;
	pthread_mutex_t	lock;
	pthread_t		thread;
	int				thread_started;
	t_dongle		*first_dongle;
	t_dongle		*second_dongle;
	t_sim			*sim;
}	t_coder;

struct s_sim
{
	t_config		config;
	t_coder			*coders;
	t_dongle		*dongles;
	pthread_t		monitor;
	pthread_mutex_t	log_mutex;
	pthread_mutex_t	state_mutex;
	int				stop;
	int				log_mutex_initialized;
	int				state_mutex_initialized;
	int				monitor_started;
	long			initialized_coders;
	long			initialized_dongle_mutexes;
	long			initialized_dongle_conds;
	long			initialized_dongle_heaps;
	long			start_ms;
	long			next_seq;
};

int		heap_init(t_heap *h, int cap);
int		heap_push(t_heap *h, t_req value);
int		heap_pop(t_heap *h, t_req *out);
int		heap_peek(const t_heap *h, t_req *out);
void	heap_free(t_heap *h);
int		heap_less(t_req a, t_req b);
void	heap_sift_up(t_heap *h, int index);
void	heap_sift_down(t_heap *h, int index);
int		parse_args(int argc, char **argv, t_config *config);
int		init_start_time(t_sim *sim);
long	now_ms(const t_sim *sim);
int		init_dongles(t_sim *sim);
void	cleanup(t_sim *sim);
int		dongle_acquire(t_dongle *dongle, t_coder *coder, t_sim *sim);
void	dongle_release(t_dongle *dongle, t_sim *sim);
int		sim_is_stopped(t_sim *sim);
int		wait_for_dongle(t_dongle *dongle, t_coder *coder, t_sim *sim);
void	dongle_wake(t_dongle *dongle);
int		coder_assign_dongles(t_sim *sim);
void	*coder_routine(void *arg);
void	*monitor_routine(void *arg);
int		log_state(t_sim *sim, long id, const char *state);
int		init_coders(t_sim *sim);
int		sleep_ms(t_sim *sim, long duration);
void	mark_compile_start(t_coder *coder);
void	count_compile(t_coder *coder);

#endif