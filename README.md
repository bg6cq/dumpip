### dumpip 记录用户的IP地址

dumpip 可以用来向用户回显IP地址，并可以将连接信息记录到文件中。

### 编译安装过程

```
cd /usr/src/
git clone https://github.com/bg6cq/dumpip.git
cd dumpip
make
```

### 回显用户的IP地址

当用户连接时，向用户显示他的IP地址。比如以下的命令执行，当用户使用`telnet`或`nc`连接服务器的90端口时，
会返回用户的IP地址。


```
/usr/src/dumpip/dumpip 90
```

### 回显用户的IP地址，并做日志记录

除以上功能外，可以将连接时间和用户的IP地址记录下来，常用来设置 蜜罐 机器，如
```
/usr/src/dumpip/dumpip 23 /var/log/23.log
/usr/src/dumpip/dumpip 3389 /var/log/3389.log
```
