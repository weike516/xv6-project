// Find Function.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
    char *p;

    //Find first character after last slash.
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
      ;
    p++;
    return p;
}

void
find(char *path, char *filename)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }//open the file
    
    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }//obtain some information about the file's stat

    switch(st.type){
        //fd is a file
        case T_FILE:
            if(strcmp(fmtname(path), filename) == 0){
                printf("%s\n", path);
            }
            break;
        // fd is a directory
        case T_DIR:
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
                printf("find: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf+strlen(buf);
            *p++ = '/';
            // open the next file
            while(read(fd, &de, sizeof(de)) == sizeof(de)){//read to dirent
                if((de.inum == 0) || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                    continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                find(buf, filename);
            }
            break;
    }
    close(fd);
}
int
main(int argc, char *argv[]){

    if(argc != 3){
        fprintf(2, "Usage: find directory filename.\n");
        exit(1);
    }
    find(argv[1], argv[2]);
    exit(0);
}

/*
#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"

#define MAX_PATH_LENGTH 512

void find(char *path, char *target) {
    char buf[MAX_PATH_LENGTH];
    int fd;
    struct dirent de;
    struct stat st;

    // 打开目录
    if ((fd = open(path, O_RDONLY)) < 0) {
        fprintf(2, "find: 无法打开目录 %s\n", path);
        return;
    }

    // 读取目录中的每个条目
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        // 跳过 "." 和 ".."
        if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
            continue;

        // 构造条目的完整路径
        snprintf(buf, sizeof(buf), "%s/%s", path, de.name);

        // 获取条目的信息
        if (stat(buf, &st) < 0) {
            fprintf(2, "find: 无法获取信息 %s\n", buf);
            continue;
        }

        // 如果条目是一个目录，则递归进入它
        if (st.type == T_DIR) {
            find(buf, target);
        }

        // 如果条目是一个文件并且名称匹配目标，则打印路径
        if (st.type == T_FILE && strcmp(de.name, target) == 0) {
            printf("%s/%s\n", path, de.name);
        }
    }

    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(2, "用法：find <目录> <文件>\n");
        exit(1);
    }

    find(argv[1], argv[2]);

    exit(0);
}

*/
