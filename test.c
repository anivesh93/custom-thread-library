#include <sys/time.h>
#include <stdio.h>

int main()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    printf("Seconds since Jan. 1, 1970: %ld\n", tv.tv_sec);
    printf("Seconds since Jan. 1, 1970: %ld\n", tv.tv_usec);
    printf("Seconds since Jan. 1, 1970: %ld\n", 1000000 * tv.tv_sec + tv.tv_usec);
    return 0;
}