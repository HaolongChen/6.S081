#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char* argv[])
{
    int p[2];
    pipe(p);
    int pid = fork();
    char buf[64];
    if(pid == 0)
    {
        close(p[0]);
        int w = dup(p[1]);
        close(p[1]);
        printf("%d: received ping\n", getpid());
        write(w, "pingpone", sizeof("pingpong"));
        close(0);
    }
    else
    {
        close(p[1]);
        int r = dup(p[0]);
        close(p[0]);
        read(r, buf, sizeof(buf));
        printf("%d: received pong\n", getpid());
        close(0);
    }
    exit(0);
}