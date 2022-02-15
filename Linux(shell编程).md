## shell编程

### 简介
#### 基本概念
shell是一个**命令解析器**，是一个程序，实际上我们也手写过shell。Linux下会安装好几种不同的shell，可以通过`cat /etc/shells`查看登录shell有哪几种：
```cpp
# /etc/shells: valid login shells
/bin/sh
/bin/bash
/bin/rbash
/bin/dash
```
也可以通过`echo $SHELL`查看当前使用的shell，一般就是`/bin/bash`，bin目录下面就是典中典的命令大全了，全是命令的二进制映像。只不过bash比较特殊，是用来**解析命令的二进制映像**。

那么其实也可以在shell里面执行另一个shell，默认的shell一般是`/bin/bash`，我们在这个shell里输入`/bin/sh`就切换到了sh这个shell，提示信息会变得不一样：

![image](https://user-images.githubusercontent.com/55400137/153984051-9eeaab91-86e9-4c08-a3da-a276ed8cad08.png)

而默认的bash的优势在于：速度快(当然其他也不慢)，支持上下键查询历史，并且可以tab自动补齐(后两条sh没有)，因此还是相当好用的。

#### shell过程

典中典的`解析命令 + fork + exec + wait`，我们手写的myshell有详细解释。

### 文件权限相关
主要是几个命令：
* chown切换文件的owner和group属性：
```cpp
chown njucs:njucs file  //切换file的用户和用户组为njucs
chown njucs:njucs dir -R  //dir是个目录，要递归切换
```
显然需要`sudo`权限才能执行。
* chmod改变文件的权限位，就是典中典的rwxrw-r--，写法有好几种：
```cpp
chmod 777 file           //直接数字位图
chmod u=rwx g+x o+w file //可以用=和+给特定用户特定权限
```
特殊的，有时候我们看到文件权限为`rws`开头而不是`rwx`，这个`s`指的是：该文件**被执行时暂时拥有root权限**，执行**完毕后恢复普通身份**，避免破坏系统，比如`ls -l /bin | grep '^...s'`查看/bin目录下的权限以`...s`开头的文件：

![image](https://user-images.githubusercontent.com/55400137/153987247-1c813d92-9eec-462c-bb79-53133fc3283f.png)

* chgrp改变用户组
```cpp
chgrp root file
chgrp root dir -R //对于目录的递归操作
```
把file的用户组改成root

有个问题，为什么默认创建文件后权限为644？而创建目录的权限为755？这其实就是`umask`掩码的影响，公式为：如果文件则权限位为`7-umask-1`，目录权限位为`7-umask`，一般`umask = 0022`。

当然还有什么符号链接和硬链接啥的，在文件系统那部分都写清楚了。

### shell脚本
shell脚本功能强大（就是说什么语言都说自己强大，然后重要的一点是节省时间，把一堆命令写到一起解析，纯纯的调包，就看你怎么用了。

#### 基本元素
第一行的#不是注释，其他地方的#都是注释。第一行`#!/bin/bash`表示用这个bash来解析命令，比如：
```sh
#!/bin/bash
hello="ZhangHuan"
echo $hello  #等同于echo ${hello}
```
要注意的是**定义变量时=前后不能有空格**，shell无法解析！还有就是**引用变量时必须以$开头**。

写完脚本后一般没有执行权限，因为默认644，手动改一下就行`chmod u+x 01hello.sh`，然后`./01hello.sh`

