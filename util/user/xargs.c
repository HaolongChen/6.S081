#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int
main(int argc, char* argv[])
{
    char buffer[256];
    int offset = 0;
    while(read(0, buffer + offset, 1) > 0){
        if(buffer[offset] == '\n'){
            char* v[50];
            for(int i = 1; i < argc; ++i){
                v[i - 1] = argv[i];
                // printf("%s\n", v[i - 2]);
            }
            // v[argc - 2] = buffer;
            // printf("%s\n", v[argc - 2]);
            // printf("%s %s\n", v[0], v[1]);
            int cnt = 0;
            int begin = 0;
            char word[256];
            // printf("%s%d\n", buffer, strlen(buffer));
            // printf("%d\n", strlen(buffer));
            for(int i = 0; i < strlen(buffer); ++i){
                if(buffer[i] == ' ' || i == strlen(buffer) - 1)
                {
                    // *v[argc - 2 + cnt++] = word;
                    v[argc - 1 + cnt] = malloc(strlen(word) + 1);
                    strcpy(v[argc - 1 + cnt], word);
                    // printf("%s\n", word);
                    cnt++;
                    for(int j = 0; j < begin; ++j) word[j] = 0;
                    begin = 0;
                }
                else
                {
                    word[begin++] = buffer[i];
                }
            }
            for(int i = 0; i <= offset; ++i) buffer[i] = 0;
            offset = 0;
            // v[argc - 2 + cnt] = 0;
            int pid = fork();
            if(pid == 0)
                exec(argv[1], v);
            else
                wait(0);
            for(int i = 0; i < cnt; ++i) free(v[argc - 1 + i]);
        }
        else offset++;
    }
    exit(0);
}