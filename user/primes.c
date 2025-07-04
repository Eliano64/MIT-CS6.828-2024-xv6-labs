#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void primes(int) __attribute__((noreturn));
void primes(int fd){
    int pivot;
    if(read(fd,&pivot,4)!=4){
        close(fd);
        exit(0);
    }

    printf("prime %d\n", pivot);

    int p[2];
    pipe(p);
    int pid=fork();
    if(pid>0){
        close(p[0]);
        int num;
        while(read(fd,&num,4)==4){
            if(num%pivot==0){
                continue;
            }
            write(p[1], &num, 4);
        }
        close(fd);
        close(p[1]);
        wait(0);
    }
    else if(pid==0){
        close(p[1]);
        close(fd);
        primes(p[0]);
    }
    exit(0);
}

int main()
{
    int p[2];
    pipe(p);
    

    int pid = fork();
    if(pid>0){
        close(p[0]);
        for(int i=2;i<=280;++i){
            write(p[1], &i, 4);
        }
        close(p[1]);
        wait(0);
    }
    else if(pid==0){
        close(p[1]);
        primes(p[0]);
    }
    exit(0);
    
}