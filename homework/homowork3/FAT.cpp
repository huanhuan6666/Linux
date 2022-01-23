// Created by ChenJensen on 16/1/1.
#include <stdio.h>
//#include <malloc/malloc.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
 
using namespace std;
 
#define BLOCKSIZE 1024
#define SIZE 1024000
#define END 65535
#define FREE 0
#define ROOTBLOCKNUM 2
#define MAXOPENFILE 10
#define FILENAME "/Users/chenjensen/Desktop/myfsys"
#define MODE "r"
 
typedef struct FCB
{
    unsigned char attribute;// 暂时只有两种属性，0：目录文件；1：数据文件
    unsigned short time;
    unsigned short data;
    unsigned short first;//该fcb块指向的第一块区域
    unsigned long length;//长度,目录项
    short  free;// 标示目录项是否为空，0为空，1为已分配
    char filename[8];//文件名
    char exname[4];//后缀名
 
}fcb;
 
//Fat表其中表示的指向下一个地址,如果没有则标记为end,没有占有的时候标记为free
typedef struct FAT
{
    unsigned short id;
}fat;
 
//当前打开目录项结构体,包含了fcb块,然后还有
typedef struct USEROPEN
{
    char filename[8];
    char exname[4];
    unsigned char attribute;// 暂时只有两种属性，0：目录文件；1：数据文件
    unsigned short time;
    unsigned short data;
    unsigned short first;
    unsigned long length;
    short free;
    
    int dirno;// 相应打开文件所在的目录项在父目录文件中的盘块号
    int diroff;// 相应打开文件的目录项在父目录文件的dirno盘块中的目录项序号
    int count;//读写指针在文件中的位置
    char dir[80];//相应打开文件所在的目录名
    short fcbstate;// 是否修改了文件的FCB的内容，1：修改；0：未修改
    short topenfile;// 标示该用户打开表项是否为空，0：空；1：已分配
}useropen;
 
typedef struct BLOCK0
{
    char information[200];
    unsigned short root;// 根目录文件的起始盘块号
    unsigned char *startblock;
}block0;
 
unsigned char *myvhard;
useropen openfilelist[MAXOPENFILE];
useropen *ptrcurdir;// 指向用户打开文件表中的当前目录所在打开文件表项的位置
char currentdir[80];
unsigned char *startp;// 虚拟磁盘上数据区开始的位置
 
void startsys();
void my_format();
void my_cd(char *dirname);
int my_open(char *filename);
int do_read(int fd,int len,char *text);
void my_close(int fd);
void my_mkdir(char *dirname);
void my_ls();
int do_write(int fd, char *text,int len,int wstyle);
void my_rmdir(char *dirname);
int my_create(char *filename);
void my_rm(char *filename);
 
/*first open the file judge if it exits
 if not exits apply an area from the memory of
 size of SIZE,
 */
 
void startsys()
{
    myvhard = (unsigned char*)malloc(SIZE);
    memset(myvhard,0,SIZE);
    FILE * fp;
    if((fp=fopen(FILENAME,MODE))!=NULL){
        fread(myvhard,SIZE,1,fp);
        fclose(fp);
    }
    else{
        printf("The file not exits,please create the file,now init the file system.\n");
        //将磁盘进行格式化
        my_format();
    }
    strcpy(openfilelist[0].filename,"root");
    strcpy(openfilelist[0].exname,"di");
    openfilelist[0].attribute = 0;
    openfilelist[0].time = ((fcb *)(myvhard+5*BLOCKSIZE))->time;
    openfilelist[0].data = ((fcb *)(myvhard+5*BLOCKSIZE))->data;
    openfilelist[0].first = ((fcb *)(myvhard+5*BLOCKSIZE))->first;
    openfilelist[0].length = ((fcb *)(myvhard+5*BLOCKSIZE))->length;
    openfilelist[0].fcbstate = 0;
    openfilelist[0].free = 1;
    openfilelist[0].topenfile = 1;
    openfilelist[0].count = 1;
    //父目录和当前目录所在的盘块号
    openfilelist[0].dirno = ((fcb *)(myvhard+5*BLOCKSIZE))->first;
    openfilelist[0].diroff = ((fcb *)(myvhard+5*BLOCKSIZE))->first;
    //当前目录指向第一项
    ptrcurdir = &openfilelist[0];
    int i;
    for (i = 1; i < MAXOPENFILE; i++) {
        strcpy(openfilelist[i].filename,"\0");
        strcpy(openfilelist[i].exname,"\0");
        openfilelist[i].attribute = 0;
        openfilelist[i].time = 0;
        openfilelist[i].data = 0;
        openfilelist[i].first = 0;
        openfilelist[i].length = 0;
        openfilelist[i].fcbstate = 0;
        openfilelist[i].free = 0;
        openfilelist[i].topenfile = 0;
        openfilelist[i].count = 1;
    }
    // 设置当前目录为根目录
    strcpy(currentdir,"\\root\\");
    strcpy(openfilelist[0].dir,currentdir);
    startp = ((block0 *)myvhard)->startblock;
}
void my_format(){
    //p指向硬盘块开始处
    unsigned char *p;
    p = myvhard;
    //时间结构体代表当前时间
    time_t *now;
    now = (time_t *)malloc(sizeof(time_t));
    //引导块
    block0 *b0;
    b0 = (block0 *)p;
    p+=BLOCKSIZE;//p当前指向1块起始地址
    //初始化出两个fat块
    fat *fat1 = (fat*)(p);
    fat *fat2 = (fat*)(p+2*BLOCKSIZE);
    //分配出的5个区域的fat表填写
    int i;
    for (i = 0; i < 5; i++,fat1++,fat2++)
    {
        fat1->id=END;
        fat2->id=END;
    }
    //开辟两个块给根目录,占掉两个fat,5,6块,第6,7块
    fat1->id=6;
    fat2->id=6;
    fat1++;
    fat2++;
    fat1->id=END;
    fat2->id=END;
    fat1++;
    fat2++;
    //此时的fat1,fat2指向下一个块了
    //将fat表中剩余几项初始化
    //??????? 初始化剩余的表项,fat占有多大???????
    for(i = 7;i < 2 * BLOCKSIZE/sizeof(fat);i++,fat1++,fat2++)
    {
        fat1->id = FREE;
        fat2->id = FREE;
    }
    //p当前指向5块,准备分给root一个块
    p += BLOCKSIZE*4;
    //在5块分配fcb
    fcb *root = (fcb *)p;
    // 设置的"." ".."
    strcpy(root->filename,".");
    strcpy(root->exname,"di");
    root->attribute = 1;
    time(now);
    
    struct tm *nowtime;
    nowtime=localtime(now);
    root->time=nowtime->tm_hour*2048+nowtime->tm_min*32+nowtime->tm_sec/2;
    root->data=(nowtime->tm_year-80)*512+(nowtime->tm_mon+1)*32+nowtime->tm_mday;
    root->first = 5;
    //其长度表示占用了两个fcb
    root->length=2*sizeof(fcb);
    //表示被占用
    root->free = 1;
    //root移动到下一个fcb块
    root++;
    strcpy(root->filename,"..");
    strcpy(root->exname,"di");
    root->attribute = 1;
    time(now);
    nowtime=localtime(now);
    root->time = nowtime->tm_hour*2048+nowtime->tm_min*32+nowtime->tm_sec/2;
    root->data = (nowtime->tm_year-80)*512+(nowtime->tm_mon+1)*32+nowtime->tm_mday;
    root->first = 5;
    root->length = 2*sizeof(fcb);
    root->free = 1;
    
    root++;
    
    // 初始化根目录区剩余的表项
    for(i = 2;i<BLOCKSIZE*2/sizeof(fcb);i++,root++)
    {
        //root->filename[0]='\0';
        root->free = 0;
    }
    
    //引导块中起始地址
    b0->startblock = p+BLOCKSIZE*4;
}
 
void my_ls(){
    // 读取当前目录文件的内容到内存
    //char *buf = (char *)malloc(1024);
    useropen *usp = &openfilelist[0];
    int i;
    for (i = 0; i < MAXOPENFILE; i++,usp++) {
        if (ptrcurdir == usp) {
            break;
        }
    }
    //根据fcb找到文件的起始块
    char *p = (char *)(myvhard + ptrcurdir->first * BLOCKSIZE);
    fcb *fileFcb = (fcb *)p;
    //fcb的块数
    int num = fileFcb->length/sizeof(fcb);
    for (i = 0; i < num; i++,fileFcb++) {
        if(fileFcb->free == 1)
            printf("filename:%s   exname:%s   time:%d:%d:%d   data:%d,%d,%d   size:%d\n",fileFcb->filename,fileFcb->exname,fileFcb->time/2048,(fileFcb->time%2048)/32,(fileFcb->time%32)*2,fileFcb->data/512+1980,fileFcb->data%512/32,
                   fileFcb->data%32,fileFcb->length);
        else i--;
    }
}
 
void my_mkdir(char *dirname){
    // 读取当前目录文件的内容到内存
    useropen *usp = &openfilelist[0];
    int i;
    //找打当前目录在fat表中对应的端口
    for (i = 0; i < MAXOPENFILE; i++,usp++) {
        if (ptrcurdir == usp)
            break;
    }
    int curOpenNum = i;
    char *buf = (char *)malloc(1024);
    do_read(i, 1024, buf);
    // 检查当前目录下新建的目录文件是否重名
    fcb *fileFcb = (fcb *)buf;
    int length = fileFcb->length;
    for (i = 0; i < length/sizeof(fcb); i++,fileFcb++) {
        if (!strcmp(fileFcb->filename, dirname)) {
            printf("目录重名\n");
            return ;
        }
        if(fileFcb->free==0)
            i--;
    }
    
    // 分配空闲文件打开表
    for (i = 0; i < MAXOPENFILE; i++) {
        if (openfilelist[i].topenfile == 0) {
            break;
        }
    }
    
    if(i==MAXOPENFILE)
    {
        printf("文件打开太多\n");
        return ;
    }
    
    int fd = i;
    // 检查FAT是否有空闲盘块
    fat *fat1 = (fat *)(myvhard + BLOCKSIZE);
    fat *fat2 = (fat *)(myvhard + BLOCKSIZE * 3);
    int freeBlock = 0;
    
    for (i = 0; i < BLOCKSIZE*2/sizeof(fat); i++,fat1++,fat2++)
    {
        if (fat1->id == FREE)
        {
            freeBlock = i;
            break;
        }
    }
    if (i == BLOCKSIZE*2/sizeof(fat))
    {
        printf("FAT满了！");
        return ;
    }
    // 在目录中分配FCB给新的目录
    fileFcb = (fcb *)buf;
    for (i = 0; i < length/sizeof(fcb); i++,fileFcb++) {
        //|| ??????!strcmp(fileFcb->filename, "..")????????
        if (!strcmp(fileFcb->filename, ".") )
            fileFcb->length += sizeof(fcb);
        if(fileFcb->free==0)
            break;
    }
    strcpy(fileFcb->filename,dirname);
    strcpy(fileFcb->exname,"di");
    fileFcb->attribute = 0;
    time_t *now = (time_t *)malloc(sizeof(time_t));
    time(now);
    struct tm *nowtime = localtime(now);
    fileFcb->time = nowtime->tm_hour*2048+nowtime->tm_min*32+nowtime->tm_sec/2;
    fileFcb->data = (nowtime->tm_year-80)*512+(nowtime->tm_mon+1)*32+nowtime->tm_mday;
    fileFcb->first = freeBlock;
    fileFcb->free = 1;
    //fileFcb->length = BLOCKSIZE;// 设置文件的长度
    fileFcb->length = 2* sizeof(fcb);
    // 在新目录分配的磁盘块中建立两个特殊的目录项"." ".."
    fcb *p = (fcb *)(myvhard + freeBlock * BLOCKSIZE);
    // 设置的"." ".."
    strcpy(p->filename,".");
    strcpy(p->exname,"di");
    p->attribute = 0;
    time(now);
    nowtime=localtime(now);
    p->time=nowtime->tm_hour*2048+nowtime->tm_min*32+nowtime->tm_sec/2;
    p->data=(nowtime->tm_year-80)*512+(nowtime->tm_mon+1)*32+nowtime->tm_mday;
    p->first = freeBlock;
    p->length = 2*sizeof(fcb);
    p->free = 1;
    p++;
    strcpy(p->filename,"..");
    strcpy(p->exname,"di");
    p->attribute = 0;
    time(now);
    nowtime=localtime(now);
    p->time = nowtime->tm_hour*2048+nowtime->tm_min*32+nowtime->tm_sec/2;
    p->data = (nowtime->tm_year-80)*512+(nowtime->tm_mon+1)*32+nowtime->tm_mday;
    p->first = ptrcurdir->first;
    p->free =1;
    p++;
    fat1->id = END;
    fat2->id = END;
    //原来的区域是要写回后才会更新的
    ptrcurdir->length += sizeof(fcb);
    for(i = 2;i<BLOCKSIZE/sizeof(fcb);i++,p++)
    {
        p->free = 0;
    }
    do_write(curOpenNum, buf, 1024, 0);
    free(buf);
}
 
void my_rm(char *filename){
    int flag;
    // 读取当前目录文件的内容到内存
    char *buf = (char *)malloc(1024);
    useropen *usp = &openfilelist[0];
    int i;
    for (i = 0; i < MAXOPENFILE; i++,usp++) {
        if (ptrcurdir == usp) {
            break;
        }
    }
    int fd = i;
    do_read(i, 1024, buf);
    // 检查要删除的目录是否存在
    fcb *fileFcb = (fcb *)buf;
    int length = fileFcb->length;
    flag = 0;
    for (i = 0; i < length/sizeof(fcb); i++, fileFcb++) {
        if (!strcmp(fileFcb->filename, filename)) {
            flag = 1;
            break;
        }
        if(fileFcb->free==0)
            i--;
    }
    if (flag == 0) {
        printf("%s目录不存在！\n",filename);
        return ;
    }
    // 回收磁盘块
    fat *fat1 = (fat *)(myvhard + BLOCKSIZE);
    fat *fat2 = (fat *)(myvhard + BLOCKSIZE * 3);
    int index = fileFcb->first;
    int indexT;
    do{
        indexT = fat1[index].id;
        fat1[index].id = FREE;
        fat2[index].id = FREE;
        index = indexT;
    }while(index!=END);
    // 清空目录项
    fileFcb->filename[0] = '\0';
    fileFcb->free = 0;
    fileFcb = (fcb *)buf;
    // 修改目录的长度
    fileFcb->length -= sizeof(fcb);
    do_write(fd, buf, 1024, 0);
    ptrcurdir->length -= sizeof(fcb);
    free(buf);
}
 
void my_rmdir(char *dirname){
    int i = 0;
    // 读取当前目录文件的内容到内存
    char *buf = (char *)malloc(1024);
    useropen *usp = &openfilelist[0];
    for (i = 0; i < MAXOPENFILE; i++,usp++) {
        if (ptrcurdir == usp) {
            break;
        }
    }
    int fd = i;
    //    curOpenNum = i;
    do_read(i, 1024, buf);
    
    // 检查要删除的目录是否存在
    fcb *fileFcb = (fcb *)buf;
    int length = fileFcb->length;
    for (i = 0; i < length/sizeof(fcb); i++,fileFcb++) {
        if (!strcmp(fileFcb->filename, dirname)) {
            break;
        }
        if(fileFcb->free==0)
            i--;
    }
    if (i==length/ sizeof(fcb)) {
        printf("目录不存在！\n");
        return ;
    }
    // 检查要删除的目录文件是否为空
    fcb *temp = (fcb *)(myvhard + BLOCKSIZE * fileFcb->first);
    if (temp->length > 2*sizeof(fcb)) {
        printf("目录不为空,确定要删除??\n");
        return ;
    }
    int flag=0;
    // 检查该目录是否已经打开
    for (i = 0; i < MAXOPENFILE; i++) {
        if (strcmp(openfilelist[i].filename, dirname) == 0) {
            flag=1;
            break;
        }
    }
    if (flag == 1) {// 把打开的目录关闭
        my_close(i);
    }
    
    // 回收磁盘块
    fat *fat1 = (fat *)(myvhard + BLOCKSIZE);
    fat *fat2 = (fat *)(myvhard + BLOCKSIZE * 3);
    int index = fileFcb->first;
    int indexT;
    //length = fileFcb->length;
    do{
        indexT = fat1[index].id;
        fat1[index].id = FREE;
        fat2[index].id = FREE;
        index = indexT;
    }while(index!=END);
    
    // 清空目录项,将其free设置为0即可,然后是将上层目录的长度进行更新,同时对于openlist中也要更新,因为其指向并不相同.
    fileFcb->filename[0] = '\0';
    fileFcb->free = 0;
    fileFcb = (fcb *)buf;
    fileFcb->length -= sizeof(fcb);
    ptrcurdir->length -= sizeof(fcb);
    do_write(fd, buf, 1024, 0);
    free(buf);
}
 
int my_create(char *filename){
    int fd;
    // 读取当前目录文件的内容到内存
    char *buf = (char *)malloc(1024);
    useropen *usp = &openfilelist[0];
    int i;
    for (i = 0; i < MAXOPENFILE; i++,usp++) {
        if (ptrcurdir == usp)
            break;
    }
    int curOpenNum = i;
    do_read(i, 1024, buf);
    // 检查当前目录下新建的目录文件是否重名
    fcb *fileFcb = (fcb *)buf;
    int length = fileFcb->length;
    for (i = 0; i < length/sizeof(fcb); i++,fileFcb++) {
        if (!strcmp(fileFcb->filename, filename)) {
            printf("文件重名！");
            return -1;
            break;
        }
        if(fileFcb->free==0)
            i--;
    }
    // 检查FAT是否有空闲盘块
    fat *fat1 = (fat *)(myvhard + BLOCKSIZE);
    fat *fat2 = (fat *)(myvhard + BLOCKSIZE * 3);
    int freeBlock = 0;
    int flag = 0;
    for (i = 0; i < BLOCKSIZE*2/sizeof(fat); i++) {
        if (fat1->id == FREE) {
            flag = 1;
            freeBlock = i;
            break;
        }
        fat1++;
        fat2++;
    }
    if (flag == 0) {
        printf("FAT满了！");
        return -1;
    }
    
    // 在目录中分配FCB给新的目录
    time_t *now;
    struct tm *nowtime;
    fileFcb = (fcb *)buf;
    now = (time_t *)malloc(sizeof(time_t));
    for (i = 0; i < length/sizeof(fcb); i++, fileFcb++){
        if (!strcmp(fileFcb->filename, "."))
            fileFcb->length += sizeof(fcb);
        if(fileFcb->free==0)
            break;
    }
    strcpy(fileFcb->filename,filename);
    strcpy(fileFcb->exname,"txt");
    fileFcb->attribute = 1;
    time(now);
    nowtime=localtime(now);
    fileFcb->time = nowtime->tm_hour*2048+nowtime->tm_min*32+nowtime->tm_sec/2;
    fileFcb->data = (nowtime->tm_year-80)*512+(nowtime->tm_mon+1)*32+nowtime->tm_mday;
    fileFcb->first = freeBlock;
    fileFcb->free = 1;
    fileFcb->length = BLOCKSIZE;// 设置文件的长度
    ptrcurdir->length += sizeof(fcb);
    do_write(curOpenNum, buf, 1024, 0);
    free(buf);
    return fd;
}
 
void my_cd(char *dirname){
    // 打开该目录文件
    int fd = my_open(dirname);
    switch (fd){
        case -1:printf("该目录为已打开的当前目录\n");break;
        case -2:printf("该文件不存在\n");break;
        case -3:printf("返回到上层目录\n");break;
        default:break;
    }
}
 
int my_open(char *filename){
    int i;
    // 读取父目录文件内容到内存
    char *buf = (char *)malloc(1024);
    useropen *usp = &openfilelist[0];
    for (i = 0; i < MAXOPENFILE; i++, usp++) {
        if (ptrcurdir == usp)
            break;
    }
    do_read(i, 1024, buf);
    // 检查欲打开文件是否存在
    fcb *fileFcb = (fcb *)buf;
    int flag = 0;
    int diroff = 0;
    int length = fileFcb->length;
    for (i = 0; i < length/sizeof(fcb); i++,fileFcb++) {
        if (!strcmp(fileFcb->filename, filename)) {
            flag = 1;
            diroff = i;
            break;
        }
    }
    if (flag == 0) {
        return -2;
    }
    // 检查文件是否已打开
    for (i = 0; i < MAXOPENFILE; i++)
        if(openfilelist[i].first==fileFcb->first){
            if (!strcmp(fileFcb->filename, "..")){
                int j=0;
                for(j=0; j<MAXOPENFILE; j++)
                    if(openfilelist[j].free==1&& (&openfilelist[j])==ptrcurdir)
                        my_close(j);
                for(j=0;j<MAXOPENFILE; j++)
                    if(openfilelist[j].free=1&& openfilelist[j].first==fileFcb->first)
                        ptrcurdir = &openfilelist[j];
                return -3;
            }
            return -1;
        }
    // 检查文件打开表是否有空表项
    flag = 0;
    for (i = 0; i < MAXOPENFILE; i++) {
        if (openfilelist[i].topenfile == 0) {
            flag = 1;
            break;
        }
    }
    int fd = i;
    if (flag == 0) {
        printf("文件打开太多！\n");
        return -1;
    }
    // 填写打开文件表表项
    strcpy(openfilelist[i].filename,fileFcb->filename);
    strcpy(openfilelist[i].exname,fileFcb->exname);
    openfilelist[i].attribute = fileFcb->attribute;
    openfilelist[i].time = fileFcb->time;
    openfilelist[i].data = fileFcb->data;
    openfilelist[i].first = fileFcb->first;
    openfilelist[i].length = fileFcb->length;
    openfilelist[i].fcbstate = 0;
    openfilelist[i].free = 1;
    openfilelist[i].topenfile = 1;
    openfilelist[i].count = 1;
    //dirno 表示的是
    openfilelist[i].dirno = ptrcurdir->first;
    openfilelist[i].diroff = diroff;
    ptrcurdir = &openfilelist[i];
    //printf("%d\n",i);
    /*
     // 设置当前目录为根目录
     strcat(currentdir, filename);
     strcat(currentdir, "\\");
     */
    free(buf);
    return fd;
}
 
void my_close(int fd){
    if(fd >= MAXOPENFILE)
    {
        printf("文件打开表溢出\n");
        return ;
    }
    strcpy(openfilelist[fd].filename,"\0");
    strcpy(openfilelist[fd].exname,"\0");
    openfilelist[fd].attribute = 0;
    openfilelist[fd].time = 0;
    openfilelist[fd].data = 0;
    openfilelist[fd].first = 0;
    openfilelist[fd].length = 0;
    openfilelist[fd].fcbstate = 0;
    openfilelist[fd].free = 0;
    openfilelist[fd].topenfile = 0;
    openfilelist[fd].count = 1;
}
 
void my_write(int fd)
{
    int writeStyle;
    char *text = (char *)malloc(BLOCKSIZE);
    if(fd>=MAXOPENFILE||fd<0)
        printf("超出所能到达范围\n");
    else{
        printf("请输入要写入的内容\n");
        scanf("%s",text);
    }
    do_write(fd,text,BLOCKSIZE,writeStyle);
    free(text);
}
 
void my_read(int fd,int len)
{
    int i;
    int j = 0;
    for (i = 0; i < MAXOPENFILE; i++) {
        if (fd == 0) {
            break;
        }
        if (openfilelist[i].topenfile == 1) {
            fd--;
        }
    }
    fd = i;
    
    char *text = (char *)malloc(BLOCKSIZE);
    if(fd>MAXOPENFILE||fd<0)
        printf("超过操作边界\n");
    else{
        //printf("%d",do_read(fd,len,text));
        printf("%s\n",text);
    }
}
 
int  do_read(int fd,int len,char *text)
{
    // 查找FAT表
    int index = openfilelist[fd].first;// 文件所在的盘块号
    unsigned char *p = openfilelist[fd].first * BLOCKSIZE + myvhard;
    int length = openfilelist[fd].length;
    //表示该项占了几个盘块
    int n = openfilelist[0].length / BLOCKSIZE + 1;
    //fat表的开始
    fat *fat1 = (fat *)(myvhard + BLOCKSIZE);
    int i,j,k = 0;
    //可能占据了多个盘块
    for (i = 0; i < n; i++) {
        for (j = 0; j < BLOCKSIZE && k < length; j++,k++,p++)
        {
            text[k] = *p;
        }
        p = myvhard + BLOCKSIZE * (fat1[index].id);
        index = fat1[index].id;
    }
    text[k] = '\0';
    return k;
}
 
int do_write(int fd,char *text,int len,int wstyle){
    int index = openfilelist[fd].first;// 文件所在的盘块号
    fat *fat1 = (fat *)(myvhard + BLOCKSIZE);
    int blkno = openfilelist[fd].first;
    unsigned char *p = blkno * BLOCKSIZE + myvhard;
    int length = ptrcurdir->length;
    int n = ptrcurdir->length / BLOCKSIZE + 1;
    int i,j,k = 0;
    for (i = 0; i < n; i++) {
        for (j = 0; j < BLOCKSIZE && k < length; j++,k++) {
            *p = text[k];
            p++;
        }
        p = myvhard + BLOCKSIZE * (fat1[index].id);
        index = fat1[index].id;
    }
    //*p = '\0';
    return 1024;
}
 
void my_exitsys(){
    FILE *fp;
    fp=fopen(FILENAME,"w");
    fwrite(myvhard,SIZE,1,fp);
    free(myvhard);
    fclose(fp);
}
 
void inputJudge()
{
    printf("........文件操作系统......\n");
    while(1){
        scanf("d");
    }
}
 
void showOpenFile(){
    int i;
    int j = 0;
    for (i = 0; i < MAXOPENFILE; i++) {
        if (openfilelist[i].topenfile == 1) {
            cout<<j<<" "<<openfilelist[i].filename<<endl;
            j++;
        }
    }
}
 
int main(int argc, const char * argv[]) {
    startsys();
    string cmd;
    int fd;
    char command[16];
    while(1){
        cout<<"C:// ";
        cin>>cmd;
        if(cmd=="mkdir"){
            cin>>command;
            my_mkdir(command);
        }
        else if(cmd=="rmdir"){
            cin>>command;
            my_rmdir(command);
        }
        else if(cmd=="ls"){
            my_ls();
        }
        else if(cmd=="cd"){
            cin>>command;
            my_cd(command);
        }
        else if(cmd=="create"){
            cin>>command;
            my_create(command);
        }
        
        else if(cmd=="write"){
            cin>>fd;
            my_write(fd);
        }
        else if(cmd=="read"){
            cin>>fd;
            my_read(fd,1024);
        }
        else if(cmd=="rm"){
            cin>>command;
            my_rm(command);
        }
        else if(cmd=="open"){
            cin>>command;
            my_open(command);
        }
        else if(cmd=="close"){
            cin>>fd;
            my_close(fd);
        }
        else if(cmd=="exit"){
            my_exitsys();
            break;
        }else if(cmd=="show"){
            showOpenFile();
        }
        else cout<<"无效指令,请重新输入:"<<endl;
    }
    return 0;
}
