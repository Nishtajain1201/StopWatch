#include<stdio.h>
#include<sys/utsname.h>

int main() {
    struct utsname system;

    uname(&system);

    printf("System name - %s \n",system.sysname);
    printf("Machine     - %s \n", system.machine);
    printf("Nodename    - %s \n", system.nodename);
    printf("Release     - %s \n", system.release);
    printf("Version     - %s \n", system.version);

    printf("\n");

    printf("Ananya \n");
    printf("Aparna \n");
    printf("Aamani \n");
    printf("Nishta \n");
    printf("Virat  \n");
    return 0;
}