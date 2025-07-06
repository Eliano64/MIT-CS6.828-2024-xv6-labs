#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

void find(char *src, char *target)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;
    if ((fd = open(src, O_RDONLY)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", src);
        return;
    }

    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", src);
        close(fd);
        return;
    }

    if (st.type == T_DIR)
    {
        strcpy(buf, src);
        p = buf + strlen(buf);
        *p++ = '/';
        while (read(fd, &de, sizeof(de)) == sizeof(de))
        {
            if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
            {
                continue;
            }
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = '\0';
            if (stat(buf, &st) < 0)
            {
                fprintf(2, "find: cannot stat %s\n", buf);
                continue;
            }

            if (st.type == T_DIR)
            {
                find(buf, target);
            }
            else if (st.type == T_FILE && strcmp(p, target) == 0)
            {
                printf("%s\n", buf);
            }
        }
        close(fd);
    }
    else
    {
        printf("find: cannot open %s as a directory\n", src);
        close(fd);
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage: find dir filename\n");
        exit(1);
    }
    find(argv[1], argv[2]);
    exit(0);
}