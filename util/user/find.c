#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  for(p=path+strlen(path); p >= path && *p != '/'; p--);
  p++;
  if(strlen(p) >= DIRSIZ) return p;

  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

void
find(char *path, char *filename)
{
    char buf[512], name[15], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }
    
    switch(st.type)
    {
        case T_FILE:
            strcpy(name, filename);
            char* cp = name + strlen(name);
            int space = strlen(fmtname(path)) - strlen(name);
            if(space > 0){
                while(space--){
                    *cp++ = ' ';
                }
            }
            if(strcmp(fmtname(path), name) == 0) {
                printf("%s\n", path);
            }
            break;

        case T_DIR:
            strcpy(buf, path);
            p = buf + strlen(buf);
            *p++ = '/';
            while(read(fd, &de, sizeof(de)) == sizeof(de)){
                if(de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                if(stat(buf, &st) < 0){
                    printf("find: cannot stat %s\n", buf);
                    continue;
                }
                find(buf, filename);
            }
            break;
    }
    close(fd);
}

int
main(int argc, char* argv[])
{
    if(argc == 3)
    {
        find(argv[1], argv[2]);
    }
    else{
        fprintf(2, "error\n");
    }
    exit(0);
}