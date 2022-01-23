#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define CMD_RESULT_BUF_SIZE 1024
#define MAX_INODE_NUM 16
/*
 * cmd：待执行命令
 * result：命令输出结果
 * 函数返回：0 成功；-1 失败；
 */
int ExecuteCMD(const char *cmd, char *result)
{
    int iRet = -1;
    char buf_ps[CMD_RESULT_BUF_SIZE];
    char ps[CMD_RESULT_BUF_SIZE] = {0};
    FILE *ptr;

    strcpy(ps, cmd);

    if((ptr = popen(ps, "r")) != NULL)
    {
        while(fgets(buf_ps, sizeof(buf_ps), ptr) != NULL)
        {
           strcat(result, buf_ps);
           if(strlen(result) > CMD_RESULT_BUF_SIZE)
           {
               break;
           }
        }
        pclose(ptr);
        ptr = NULL;
        iRet = 0;  // 处理成功
    }
    else
    {
        printf("popen %s error\n", ps);
        iRet = -1; // 处理失败
    }

    return iRet;
}
typedef struct inode
{
    int used;
    int count;
	int num;
    char names[1024];
    int end_index;
}inode;
inode inodes[MAX_INODE_NUM];
void solution_output(char *result)
{
    int i = 0;
    char the_name[1024];
    char the_num[1024];
    int flag = 0;
    for(i = 0; i < strlen(result); i++)
    {
        int k = 0;
        for(k =0; result[i] != ' '; k++, i++)
            the_num[k] = result[i];
        the_num[k] = '\0';
        int real_num = atoi(the_num);
        int if_have = 0;
        for(int j = 0; j < MAX_INODE_NUM; j++)
        {   
               if(inodes[j].used == 1)
               {
                   if(inodes[j].num == real_num)
                   {
                    	i++;//result[i] -> ' '
                    	if_have = 1;
                    	inodes[j].count++;
                    	if(inodes[j].names[inodes[j].end_index] == '\0')
                    	{//change the last char '\0' to '\n' make a longer str
                        	inodes[j].names[inodes[j].end_index] = ' ';
                        	inodes[j].end_index ++;
                    	}
                    	for(;result[i] != '\n'; i++, inodes[j].end_index++)
                        	inodes[j].names[inodes[j].end_index] = result[i];
                    	inodes[j].names[inodes[j].end_index] = '\0';
                    	//append the name in this num inode
                    	break;
                    }
                }
        }
        if(if_have == 0)//this num don't in inodes 
        {
            int m = 0;
            for(m = 0; m < MAX_INODE_NUM; m++)//find a free inode
                if(inodes[m].used == 0)
                    break;
            inodes[m].num = real_num;
            inodes[m].used = 1;//allocate a inode to this num
            inodes[m].count++;
            i++;//result[i] -> ' '
            for(;result[i] != '\n'; i++, inodes[m].end_index++)
                inodes[m].names[inodes[m].end_index] = result[i];
            inodes[m].names[inodes[m].end_index] = '\0';
            //append the name in this num inode
        }
   }
}
int main()
{
    char result[CMD_RESULT_BUF_SIZE]={0};
    for(int i = 0; i < MAX_INODE_NUM; i++)
    {
        inodes[i].count = 0;
        inodes[i].end_index = 0;
        inodes[i].end_index = 0;
    }
    ExecuteCMD("ls -i", result);
    printf("the terminal ouput:\n\n");
    printf("%s", result);
    printf("===============\n");
    solution_output(result);
    printf("\nThis is the end anwser\n");
    printf("|  inode num\ti-count\t    flies\n");
    for(int p = 0; p < MAX_INODE_NUM; p++)
    {
        if(inodes[p].count > 1)
        {
            printf(">  %d\t", inodes[p].num);
            printf("   %d\t", inodes[p].count);
            printf(" %s \n", inodes[p].names);
        }
    }
    printf("\n===============\n");
    return 0;
}
