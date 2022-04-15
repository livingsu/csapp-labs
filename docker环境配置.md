# docker环境配置

## docker

下载docker desktop，命令行：

```
docker pull ubuntu
```

会拉取ubuntu的image，然后run就会创建一个container（比如name是MyUbuntu）



## basic dependence

以root身份运行container：

```
docker exec -u 0 -it MyUbuntu bash
```

1. 下载svn：

```
apt-get update
apt-get install subversion
```

2. svn获取：

```
svn checkout svn://ipads.se.sjtu.edu.cn/ics-se20/ics518021910888
```

3. 下载vim

```
apt install vim
```

4. 下载编译器：

```
apt update
apt upgrade
apt install build-essential
```

5. 上传文件：

```
svn commit filename -m 'message'
```

