//Xargs Funtion.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

int
main(int argc, char *argv[])
{
    char buf[512];
    char *full_argv[MAXARG];
    int i;
    int len;
    if(argc < 2){
        fprintf(2, "Usage: xargs command\n");
        exit(1);
    }
    if(argc + 1 > MAXARG){
        fprintf(2, "Usage: too many args\n");
        exit(1);
    }

    for(i=1 ; i < argc ; i++){
        full_argv[i-1] = argv[i];
    }

    full_argv[argc] = 0;

    while (1) {
        i = 0;
        while (1) {
            len = read(0, &buf[i], 1);
            if(len <= 0 || buf[i] == '\n') break;
            i++;
        }
        if(i == 0) break;
        buf[i] = 0;
        full_argv[argc-1]=buf;
        if(fork() == 0){
            exec(full_argv[0], full_argv);
            exit(0);
        } else {
            wait(0);
        }
    }
    exit(0);
}
/*
#include "kernel/types.h"
#include "user/user.h"

#define MAXARG 20

int main(int argc, char *argv[]) {
    int nargs = 0; // 当前已读取的参数个数
    char *cmd[MAXARG + 1]; // 用于存储命令及其参数的数组
    char c; // 当前正在读取的字符

    // 检查是否有命令行参数
    if (argc <= 1) {
        fprintf(2, "用法：xargs 命令...\n");
        exit(1);
    }

    // 初始化 cmd 数组，将第一个命令作为要执行的命令
    cmd[nargs++] = argv[1];

    // 从标准输入读取字符，直到出现换行符（行尾）
    while (read(0, &c, 1) > 0) {
        // 检查当前字符是否是空格或换行符
        if (c == ' ' || c == '\n') {
            // 以空字符作为参数结束符
            cmd[nargs] = 0;

            // 创建子进程执行命令
            int pid = fork();
            if (pid < 0) {
                fprintf(2, "xargs: 创建子进程失败\n");
                exit(1);
            } else if (pid == 0) {
                // 在子进程中使用 exec 调用来执行命令及其参数
                exec(cmd[0], cmd);

                // 如果 exec 调用返回，表示命令执行失败
                fprintf(2, "xargs: 执行命令失败\n");
                exit(1);
            } else {
                // 在父进程中等待子进程执行完毕
                wait(0);

                // 重置 nargs 以准备执行下一个命令
                nargs = 1;
            }
        } else {
            // 检查是否超过最大参数个数
            if (nargs >= MAXARG) {
                fprintf(2, "xargs: 参数过多\n");
                exit(1);
            }

            // 将当前字符作为参数的一部分
            if (c != ' ' && c != '\n')
                cmd[nargs++] = argv[1];
        }
    }

    // 以空字符作为参数结束符
    cmd[nargs] = 0;

    // 创建子进程执行最后一个命令
    int pid = fork();
    if (pid < 0) {
        fprintf(2, "xargs: 创建子进程失败\n");
        exit(1);
    } else if (pid == 0) {
        // 在子进程中使用 exec 调用来执行最后一个命令及其参数
        exec(cmd[0], cmd);

        // 如果 exec 调用返回，表示命令执行失败
        fprintf(2, "xargs: 执行命令失败\n");
        exit(1);
    } else {
        // 在父进程中等待子进程执行完毕
        wait(0);
    }

    exit(0);
}

*/
