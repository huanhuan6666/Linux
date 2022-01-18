实现如下：
```cpp
#include<stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include<glob.h>
#include<string.h>

int notloop(const char *path)
{
	//path maybe so long like:/aaa/bbb/ccc/ddd/.
	char *pos = strrchr(path, '/');
	if(	strcmp(pos+1, ".") == 0 ||
		strcmp(pos+1, "..") == 0)
		return 0;
	return 1;
}
int64_t mydu(const char *path)
{
	struct stat statres;
	glob_t globres;
	char dirname[1024];
	int i;
	int64_t sum;
	if(lstat(path, &statres) != 0)
	{
		perror("lstat");
		exit(1);
	}
	if(!S_ISDIR(statres.st_mode)) //end 
	{
		return statres.st_blocks;
	}
	//is a dictionary, use glob() to analyse the dir
	sprintf(dirname, "%s/*", path); 
	glob(dirname, 0, NULL, &globres);
	sprintf(dirname, "%s/.*", path); //hiden file
	glob(dirname, GLOB_APPEND, NULL, &globres);

	sum = 0;
	for(i = 0; i<globres.gl_pathc; i++)
	{
		if(notloop(globres.gl_pathv[i]))
		{
			sum += mydu(globres.gl_pathv[i]);	
		}
	}
	sum += statres.st_blocks; //add file itself
	return sum;

}
int main(int argc, char **argv)
{
	if(argc < 2)
	{
		fprintf(stderr, "%s Usage: too fewer arguments\n", argv[0]);
		exit(1);
	}

	int64_t size = mydu(argv[1]);
	printf("%ld\n", size/2);
	exit(0);
}
```
* 递归时要判断不要循环，因为Linux目录结构不是单纯的多叉树结构，它有.和..两个是向回指的，如果这样的递归的话就无法结束了
* 递归的优化：对于递归考虑将递归函数栈尽可能小，显然参数和返回地址是无法改变的，因此尽可能将一些变量**放到静态区而不是局部变量**，规则如下：
  * 变量如果可以全部放在递归点之前或者递归点之后，那么就可以优化成静态变量，因为**不用担心它被子过程所修改**。

因此上面代码可以把`stat statres`的使用放在递归点之前，然后就可以优化成静态变量了。
```cpp
int64_t mydu(const char *path)
{
  static struct stat statres; //静态区优化
	glob_t globres;
	static char dirname[1024]; //静态区优化
	int i;
	int64_t sum;
	if(lstat(path, &statres) != 0)
	{
		perror("lstat");
		exit(1);
	}
	if(!S_ISDIR(statres.st_mode)) //end 
	{
		return statres.st_blocks;
	}
	//is a dictionary, use glob() to analyse the dir
	sprintf(dirname, "%s/*", path); 
	glob(dirname, 0, NULL, &globres);
	sprintf(dirname, "%s/.*", path); //hiden file
	glob(dirname, GLOB_APPEND, NULL, &globres);

	sum = statres.st_blocks; //放到递归点之前方便优化
	for(i = 0; i<globres.gl_pathc; i++)
	{
		if(notloop(globres.gl_pathv[i]))
		{
			sum += mydu(globres.gl_pathv[i]);	
		}
	}
	return sum;

}
```
