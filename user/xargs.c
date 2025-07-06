#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    char *args[MAXARG], buf[512];
    for (int i = 1; i < argc; ++i)
    {
        args[i - 1] = argv[i];
    }
    int len = 0;
    int ptr = argc-1;
    char c;
    while (read(0, &c, 1))
    { // stdin: 0
        if (c == ' ' || c == '\n')
        {
            if (len)
            {
                buf[len] = 0;
                args[ptr] = (char *)malloc(len + 1);
                len = 0;
                strcpy(args[ptr++], buf);
            }
        }
        else
        {
            buf[len++] = c;
        }

        if( c == '\n')// exec for each line in stdin
        {
            args[ptr]=0;// argv in int exec(char *path, char **argv) should end with 0 for "for(argc =0; argv[argc]; argc++)" in source code exec.c
            int pid = fork();
            if(pid>0){
                wait(0);
            }
            else if(pid==0){
                exec(args[0],args);
            }
            for(int i=argc-1;i<ptr;++i){
                free(args[i]);
            }
            ptr = argc - 1;
            
        }
        
    }
    
    exit(0);
}
