#include "codexion.h"


int in_range_int(char   *s)
{
    long long nb = 0;
    long i = 0;

    while (s[i])
    {
        nb = nb * 10 + (s[i] - '0');

        if (nb > 2147483647)
        {
            return (0);
        }
        i++;
    }
    return (1);
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

void parse_arguments(int argc, char **argv)
{
    int i;
    i = 1;

    if (argc != 9)
    {
        printf("The number of arguments its not correct.");
        exit(-1);
    }
    while (i < 8)
    {
        if (!check_is_valid_number(argv[i]))
        {
            printf("The argument number %d isn't valid it should be an positive integer.", i + 1);
            exit(-1);
        }
        if (!in_range_int(argv[i]))
        {
            printf("The argument number %d exceeds the max int value.\n", i);
            exit(-1);
        }
        i++;
    }
    if (ft_strcmp(argv[8], "fifo") != 0 && ft_strcmp(argv[8], "edf") != 0)
    {
        printf("The scheduler should be exactly edf or fifo.");
        exit(-1);
    }  
}

int main(int argc, char **argv)
{
    parse_arguments(argc, argv);
    return 0;
}