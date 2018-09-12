# C-HTTP-Server
An improved version after learning operating system for a semester...now truly supports multi-thread and gzip function...

## Original Demands
httpd [OPTION]... DIR

启动一个http服务器，监听端口由-p或--port指定，响应HTTP GET请求。URL的/被映射到DIR目录：

    如果URL对应某个目录，例如/或/dir_name，或/dir_name/，返回目录下的index.html文件；
    支持.和..的相对路径解析，例如/dir_name/..等同于/；
    收到SIGINT信号 (终端中的Ctrl-C)后释放所有资源、安全退出。
This is the last mini-lab assignment in the NJU-2018-Spring-OS class. 

## Other things
Running on linux; truly multi-threaded; gzip enabled, which could improve the speed for transmission
/site and /my for testing.
