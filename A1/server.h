#ifndef SERVER_H
#define SERVER_H
#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<errno.h>
#include<string>
#include<fcntl.h>
#include<assert.h>
#include<signal.h>
#include<sys/wait.h>
#include <string>
#include <unordered_map>
#include <set>
#include <mutex>
#ifdef __linux__
#include <sys/epoll.h>
#elif __APPLE__
#include <sys/event.h>
#endif

#endif
