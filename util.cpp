#include <stdio.h>  
#include<sys/time.h>
#include<time.h>
 
#include "util.h"

unsigned int get_msec(void)
{
    static struct timeval timeval, first_timeval;

    gettimeofday(&timeval, 0);

    if (first_timeval.tv_sec == 0) {
        first_timeval = timeval;
        return 0;
    }
    return (timeval.tv_sec - first_timeval.tv_sec) * 1000 + (timeval.tv_usec - first_timeval.tv_usec) / 1000;
}

