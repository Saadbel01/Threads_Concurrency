#ifndef CODEXION_H
# define CODEXION_H

# include <stdlib.h>
# include <stdio.h>
# include <unistd.h>
# include <fcntl.h>
# include <pthread.h>
# include <sys/time.h>


typedef struct s_shared t_shared;
typedef struct s_coder  t_coder; 

typedef struct  t_arg
{
    int nb_coders;
    int time_to_burnout;
    int time_to_compile;
    int time_to_debug;
    int time_to_refactor;
    int nb_of_compiles;
    int dongle_cooldown;
    int scheduler;

}t_arg;

typedef struct t_scheduler
{
    int state;
}t_scheduler;


typedef struct  t_dongle
{
    int dongle_id;
    int is_held;
    long available_at;
    pthread_mutex_t lock;
    pthread_cond_t cond;
}t_dongle;

typedef struct s_coder
{
    int coder_id;
    int compiles_done;
    int phase;
    int left_dongle;
    int right_dongle;
    long last_compile_start;
    t_shared *shared;
}t_coder;

typedef struct s_shared
{
    t_arg *args;
    t_dongle *dongle_array;
    t_coder *coders;
    int stop_simulation;
    pthread_mutex_t mutex_stop;
    pthread_mutex_t log_mutex;
    long start_simulation;
    t_scheduler *scheduler_state;
}t_shared;


void get_arguments(char **argv, t_arg    *args);
int     ft_strcmp(const char	*s1, const char	*s2);
int     ft_strlen(const char    *str);
long	get_time_ms(void);

#endif