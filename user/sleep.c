#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
   // 检查是否提供了sleep时间参数
    if (argc < 2)
    {
        fprintf(2, "Usage: sleep ticks\n");
        exit(1);
    }

    // 将命令行参数转换为整数
    int ticks = atoi(argv[1]);

    // 调用系统调用sleep进行暂停
    sleep(ticks);

    // 执行完毕，退出程序
    exit(0);
}
