#ifndef CODEXION_H
# define CODEXION_H

# define _DEFAULT_SOURCE

# include <stdlib.h>
# include <stdio.h>
# include <unistd.h>
# include <fcntl.h>
# include <pthread.h>
# include <sys/time.h>
# include <time.h>

# define FIFO 0
# define EDF 1
# define HEAP_CAPACITY 2
# define TOP 0
# define SECOND 1

typedef struct s_shared	t_shared;
typedef struct s_coder	t_coder;

typedef struct s_request
{
	int			coder_id;
	long long	arrival_time;
	long long	deadline;
}	t_request;

typedef struct s_arg
{
	int	nb_coders;
	int	time_to_burnout;
	int	time_to_compile;
	int	time_to_debug;
	int	time_to_refactor;
	int	nb_of_compiles;
	int	dongle_cooldown;
	int	scheduler;
}	t_arg;

typedef struct s_dongle
{
	t_request		heap[HEAP_CAPACITY];
	int				heap_size;
	int				dongle_id;
	int				is_held;
	long long		available_at;
	pthread_mutex_t	lock;
	pthread_cond_t	cond;
}	t_dongle;

typedef struct s_coder
{
	int				coder_id;
	int				compiles_done;
	int				phase;
	int				left_dongle;
	int				right_dongle;
	long long		last_compile_start;
	pthread_mutex_t	compile_lock;
	t_shared		*shared;
}	t_coder;

typedef struct s_shared
{
	t_arg			*args;
	t_dongle		*dongle_array;
	t_coder			*coders;
	int				stop_simulation;
	pthread_mutex_t	mutex_stop;
	pthread_mutex_t	log_mutex;
	long long		start_simulation;
}	t_shared;

int			in_range_int(char *s);
int			check_is_valid_number(char *s);
void		check_arguments(int argc, char **argv);
void		get_arguments(char **argv, t_arg *args);

int			init_dongle_array(t_dongle *array, int nb_coders);
int			init_coders(t_coder *coders, t_shared *shared, int nb_coders);
t_shared	*init_shared(t_arg *args);

void		compling(t_coder *coder);
void		debugging(t_coder *coder);
void		refactoring(t_coder *coder);
void		*coder_routine(void *arg);

void		*monitor_routine(void *arg);

int			acquire_dongles(t_coder *coder);
void		release_dongle(t_dongle *dongle, int cooldown);
void		release_dongles(t_coder *coder);
void		push_both_requests(t_coder *coder, t_dongle *d1, t_dongle *d2);
void		remove_both_requests(t_coder *coder, t_dongle *d1, t_dongle *d2);
int			handle_single_coder(t_coder *coder);

int			is_higher_priority(t_request a, t_request b, int scheduler);
void		heap_push(t_dongle *dongle, t_request request, int scheduler);
t_request	heap_pop(t_dongle *dongle);
void		heap_remove(t_dongle *dongle, int coder_id);

int			ft_strcmp(const char *s1, const char *s2);
int			ft_strlen(const char *str);
long long	get_time_ms(void);
int			simulation_stopped(t_shared *shared);
void		safe_sleep(long ms, t_shared *shared);
void		wake_all_dongles(t_shared *shared);
void		print_log(t_coder *coder, char *msg);

#endif