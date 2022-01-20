#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <glob.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#define DELMIS " \t\n"
void prompt()
{
  printf("myshell-0.1$ ");
}

void parse (char *line, glob_t *res)
{
  char *token;
  int i = 0;
  while(1)
  {
    token = strsep(&line, DELMIS); //用strsep将命令分隔
    if(token == NULL)
      break;
    if(token[0] == '\0') //解析出空串则继续
      continue;
    glob(token, GLOB_NOCHECK|GLOB_APPEND*i, NULL, res); //res是个glob_t输出指针，第二次开始GLOB_APPEND生效
    i = 1;
  }
}

int main()
{
  int pid = 0;
  glob_t res;
  char *bufline = NULL;
  size_t bufline_size = 0;
  while(1)
  {
    prompt(); //输出提示符
    getline(&bufline, &bufline_size, stdin); //输入命令
    parse(bufline, &res); //解析命令
    if(0)
    {
        //内部命令不做处理
    }
    else //外部命令
    {
      pid = fork(); //创建子进程
      if(pid < 0)
      {
        perror("fork()");
        exit(1);
      }
      else if(pid == 0) //子进程
      {
        execvp(res.gl_pathv[0], res.gl_pathv); //文件名及参数列表
        exit(0); //执行完退出
      }
      else //父进程
        wait(NULL); //等待子进程结束
    }
  }
}
