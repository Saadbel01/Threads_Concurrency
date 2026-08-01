#ifndef CODEXION_H
# define CODEXION_H

# include <stdlib.h>
# include <stdio.h>
# include <unistd.h>
# include <fcntl.h>

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


int	ft_atoi(const char	*str);
int	ft_strcmp(const char	*s1, const char	*s2);
int ft_strlen(const char    *str);
void get_arguments(char **argv, t_arg    *args);

#endif