#include "codexion.h"


int in_range_int(char   *s)
{
    long long nb = 0;
    long i = 0;

    while (s[i])
    {
        nb = nb * 10 + (s[i] - '0');

        if (nb > 2147483647 || nb == 0)
        {
            return (0);
        }
        i++;
    }
    return nb;
}

int check_is_valid_number(char  *s)
{
    int i;
    i = 0;
    while (s[i])
    {
        if (s[i] >= '0' && s[i] <= '9')
            i++;
        else
            return (0);
    }
    return (1);
}

void check_arguments(int argc, char **argv)
{
    int i;
    i = 1;

    if (argc != 9)
    {
        printf("The number of arguments its not correct.");
        exit(1);
    }
    while (i < 8)
    {
        if (!check_is_valid_number(argv[i]) )
        {
            printf("The argument number %d isn't valid, it should be an positive integer.", i + 1);
            exit(1);
        }
        if (!in_range_int(argv[i]))
        {
            printf("The argument number %d should be between 1 and MAX_INT.\n", i);
            exit(1);
        }
        i++;
    }
    if (ft_strcmp(argv[8], "fifo") != 0 && ft_strcmp(argv[8], "edf") != 0)
    {
        printf("The scheduler should be exactly edf or fifo.");
        exit(1);
    }  
}

void get_arguments(char **argv, t_arg    *args)
{
    args->nb_coders = in_range_int(argv[1]);
    args->time_to_burnout = in_range_int(argv[2]);
    args->time_to_compile = in_range_int(argv[3]);
    args->time_to_debug = in_range_int(argv[4]);
    args->time_to_refactor = in_range_int(argv[5]);
    args->nb_of_compiles = in_range_int(argv[6]);
    args->dongle_cooldown = in_range_int(argv[7]);
    if (ft_strcmp(argv[8], "fifo") == 0)
        args->scheduler = 0;
    else
        args->scheduler = 1;
}

int main(int argc, char **argv)
{
    check_arguments(argc, argv);
    t_arg   *arguments;
    arguments = malloc(sizeof(t_arg));

    if (!arguments)
    {
        printf("Failed to allocate memory for arguments.");
        exit(-1);
    }

    get_arguments(argv, arguments);
    printf("this: %d\n", arguments->scheduler);
    return 0;
}