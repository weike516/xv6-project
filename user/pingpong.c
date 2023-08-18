#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int p1[2], p2[2];
    char buf[] = {'A'}; // 一个字节
    int n = sizeof(buf);

    // 创建两个管道
    pipe(p1);
    pipe(p2);

    // 创建子进程
    if(fork() == 0) {

        close(p1[1]); // 关闭子进程不使用的写入端
        close(p2[0]); // 关闭子进程不使用的读取端

        // 从管道p1读取数据，并检查读取是否成功
        if(read(p1[0],buf,n) != n){
            printf("Error: child read from p1 with parent failed.");
            exit(1);
        }

        // 打印子进程的进程ID和收到的信息
        printf("%d: received ping\n", getpid());

        // 向管道p2写入数据，并检查写入是否成功
        if(write(p2[1],buf,n) != n){
            printf("Error: child write into p2 with parent failed.");
            exit(1);
        }

        // 子进程退出
        exit(0);
    } else {
        close(p1[0]); // 关闭父进程不使用的读取端
        close(p2[1]); // 关闭父进程不使用的写入端

        // 向管道p1写入数据，并检查写入是否成功
        if(write(p1[1],buf,n) != n){
            printf("Error: parent write into p1 with child failed.");
            exit(1);
        }

        // 从管道p2读取数据，并检查读取是否成功
        if(read(p2[0],buf,n) != n){
            printf("Error: parent read from p2 with child failed.");
            exit(1);
        }

        // 打印父进程的进程ID和收到的信息
        printf("%d: received pong\n",getpid());

        // 等待子进程退出
        wait(0);

        // 父进程退出
        exit(0);
    }
}

/*
#include "kernel/types.h"
#include "user/user.h"

int main() {
    int parent_to_child[2]; // Pipe from parent to child
    int child_to_parent[2]; // Pipe from child to parent
    char buf;
    int pid;

    // Create pipes
    if (pipe(parent_to_child) < 0 || pipe(child_to_parent) < 0) {
        fprintf(2, "Pipe creation failed\n");
        exit(1);
    }

    // Fork a child process
    if ((pid = fork()) < 0) {
        fprintf(2, "Fork failed\n");
        exit(1);
    }

    if (pid == 0) {
        // Child process
        close(parent_to_child[1]); // Close write end of pipe from parent to child
        close(child_to_parent[0]); // Close read end of pipe from child to parent

        // Read from parent and print "<pid>: received ping"
        if (read(parent_to_child[0], &buf, 1) != 1) {
            fprintf(2, "Read from parent failed\n");
            exit(1);
        }
        printf("%d: received ping\n", getpid());

        // Write to parent and exit
        if (write(child_to_parent[1], &buf, 1) != 1) {
            fprintf(2, "Write to parent failed\n");
            exit(1);
        }
        exit(0);
    } else {
        // Parent process
        close(parent_to_child[0]); // Close read end of pipe from parent to child
        close(child_to_parent[1]); // Close write end of pipe from child to parent

        // Write to child
        buf = 'a';
        if (write(parent_to_child[1], &buf, 1) != 1) {
            fprintf(2, "Write to child failed\n");
            exit(1);
        }

        // Read from child and print "<pid>: received pong"
        if (read(child_to_parent[0], &buf, 1) != 1) {
            fprintf(2, "Read from child failed\n");
            exit(1);
        }
        printf("%d: received pong\n", getpid());
    }

    exit(0);
}

*/

