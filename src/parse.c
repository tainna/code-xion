#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "codexion.h"

static int	parse_number(const char *s, long *out)
{
	long	res;
	int		digit;

	if (s == NULL || *s == '\0')
		return (0);
	res = 0;
	while (*s != '\0')
	{
		if (*s < '0' || *s > '9')
			return (0);
		digit = *s - '0';
		if (res > (LONG_MAX - digit) / 10)
			return (0);
		res = res * 10 + digit;
		s++;
	}
	*out = res;
	return (1);
}

static int	read_arg(const char *s, long min, long *out)
{
	if (!parse_number(s, out) || *out < min)
		return (0);
	return (1);
}

static int	parse_scheduler(const char *s, t_scheduler *out)
{
	if (strcmp(s, "fifo") == 0)
		*out = CODEXION_SCHED_FIFO;
	else if (strcmp(s, "edf") == 0)
		*out = CODEXION_SCHED_EDF;
	else
		return (0);
	return (1);
}

static int	fail(const char *msg)
{
	fprintf(stderr, "Error: %s\n", msg);
	return (0);
}

int	parse_args(int argc, char **argv, t_config *c)
{
	if (argc != 9)
		return (fail("expected 8 arguments"));
	if (!read_arg(argv[1], 1, &c->number_of_coders)
		|| !read_arg(argv[2], 1, &c->time_to_burnout)
		|| !read_arg(argv[3], 1, &c->time_to_compile)
		|| !read_arg(argv[4], 0, &c->time_to_debug)
		|| !read_arg(argv[5], 0, &c->time_to_refactor)
		|| !read_arg(argv[6], 1, &c->compiles_required)
		|| !read_arg(argv[7], 0, &c->dongle_cooldown))
		return (fail("invalid numeric argument"));
	if (!parse_scheduler(argv[8], &c->scheduler))
		return (fail("scheduler must be fifo or edf"));
	return (1);
}