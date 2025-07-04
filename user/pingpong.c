#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main()
{
    int pid = fork();
    if(pid==-1){
        exit(-1);
    }
    int p1[2]; // 父进程->子进程
    int p2[2]; // 子进程->父进程
    pipe(p1);
    pipe(p2);
    char buf;
    if(pid>0){
        close(p1[0]); // 关闭p1读端
        close(p2[1]); // 关闭p2写端
        write(p1[1], "a", 1);
        close(p1[1]);
        read(p2[0],&buf,1);
        close(p2[0]);
        wait(0);
        fprintf(2, "%d: received pong\n", getpid());
    }
    else{
        close(p1[1]);
        close(p2[0]);

        read(p1[0],&buf,1);
        close(p1[0]);
        fprintf(2, "%d: received ping\n", getpid());
        write(p2[1],"a",1);
        close(p2[1]);
    }
    exit(0);
}