#include "codexion.h"


int is_higher_priority(t_request a, t_request b, int scheduler)
{
    if (scheduler == FIFO)
        return (a.arrival_time < b.arrival_time);
    else if (scheduler == EDF)
        return (a.deadline < b.deadline);
    return (0);
}

void    heap_push(t_dongle *dongle, t_request request, int scheduler)
{
    int         index;
    t_request   tmp;

    if (dongle->heap_size >= 2)
        return;

    index = dongle->heap_size;
    dongle->heap[index] = request;
    dongle->heap_size++;

    if (index == 1
        && is_higher_priority(dongle->heap[1],
            dongle->heap[0], scheduler))
    {
        tmp = dongle->heap[0];
        dongle->heap[0] = dongle->heap[1];
        dongle->heap[1] = tmp;
    }
}

t_request heap_pop(t_dongle *dongle)
{
    t_request request;

    request = dongle->heap[0];

    if (dongle->heap_size == 0)
        return (request);

    dongle->heap_size--;

    if (dongle->heap_size > 0)
        dongle->heap[0] = dongle->heap[dongle->heap_size];

    return (request);
}

void heap_remove(t_dongle *dongle, int coder_id)
{
    int i;

    i = 0;
    while (i < dongle->heap_size)
    {
        if (dongle->heap[i].coder_id == coder_id)
        {
            dongle->heap_size--;

            if (i != dongle->heap_size)
                dongle->heap[i] = dongle->heap[dongle->heap_size];

            return;
        }
        i++;
    }
}