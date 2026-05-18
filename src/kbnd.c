/*
 * kbnd.c - Kernel Bypass Network Driver
 *
 * Justus Languell  <jus@justusl.com>
 */

#include <stdio.h>
#include <sys/utsname.h>


int main(int argc, char** argv)
{
    struct utsname u;

    printf("Hello, world!\n");

    if (uname(&u) == 0)
    {
        printf("Host %s\tKernel %s\tArch %s\n", u.nodename, u.release, u.machine);
    } 
    else 
    {
        perror("uname");
        fprintf(stderr, "Failed to grab uname\n");
    }
}



