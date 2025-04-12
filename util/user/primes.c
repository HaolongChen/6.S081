#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char* argv[])
{
    int p[2];
    pipe(p);
    int pid = fork();
    int prime = 2;
    int buf[40] = {};
    int left = 34;
    for(int i = 0; i < 34; ++i) buf[i] = i + 2;
    while(pid == 0)
    {
        int neighbour[40];
        int size = 0;
        close(p[1]);
        while(read(p[0], &neighbour[size], 4)) size++;
        if(!size){
            close(p[0]);
            exit(0);
        }
        close(p[0]);
        prime = neighbour[0];
        printf("prime %d\n", prime);
        left = 0;
        for(int i = 1; i < size; ++i){
            if(neighbour[i] % prime) buf[left++] = neighbour[i];
        }
        pipe(p);
        pid = fork();
    }
    close(p[0]);
    for(int i = 0; i < left; ++i){
        write(p[1], &buf[i], 4);
    }
    close(p[1]);
    wait(0);
    exit(0);
}