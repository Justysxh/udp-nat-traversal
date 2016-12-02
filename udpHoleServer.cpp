/**
by xiaohuh421@qq.com  
//*/

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>


void udpHoleServer();

int main()
{
   udpHoleServer();
   return 0;
}

class CAutoCloseSocket
{
public:
    int mFD;
    CAutoCloseSocket(int fd)
    {
        mFD = fd;
    }
    ~CAutoCloseSocket()
    {
        close(mFD);
    }
};


void udpHoleServer()
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd==-1)
    {
        printf("create socket failed\n");
        return;
    }
    CAutoCloseSocket sock(fd);
    int port = 18901;
    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family      = AF_INET;
    my_addr.sin_port        = htons(port);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    
    int bindret = bind(fd, (const struct sockaddr *)&my_addr,sizeof(my_addr));
    if(bindret==-1)
    {
        printf("bind failed error:%d\n", errno);
        return;
    }
    
    sockaddr_in clientaddr[2]={0};
    int clients = 0;
    ssize_t ret = 0;
    char buf[0x10]={0};
    while(true)
    {
        printf("wait for client...\n");
        socklen_t addrlen = sizeof(sockaddr_in);
        ret = recvfrom(fd, buf, 10, 0, (struct sockaddr *)&clientaddr[clients], &addrlen);
        if(ret == -1)
        {
            printf("recvfrom failed error:%d\n", errno);
            break;
        }
        printf("client come: %s:%zd\n", inet_ntoa(clientaddr[clients].sin_addr), clientaddr[clients].sin_port);
        ++clients;
        if(clients==2)
        {
            clients = 0;
            printf("send addr to client\n");
            char sendBuf[0x20]={0};
            *(int*)sendBuf = clientaddr[0].sin_addr.s_addr;
            *(short*)&sendBuf[4] = clientaddr[0].sin_port;
            *(int*)&sendBuf[6] = clientaddr[1].sin_addr.s_addr;
            *(short*)&sendBuf[10] = clientaddr[1].sin_port;
            sendto(fd, sendBuf, 12, 0, (sockaddr*)&clientaddr[1], sizeof(sockaddr_in));
            
            *(int*)sendBuf = clientaddr[1].sin_addr.s_addr;
            *(short*)&sendBuf[4] = clientaddr[1].sin_port;
            *(int*)&sendBuf[6] = clientaddr[0].sin_addr.s_addr;
            *(short*)&sendBuf[10] = clientaddr[0].sin_port;
            sendto(fd, sendBuf, 12, 0, (sockaddr*)&clientaddr[0], sizeof(sockaddr_in));
            
        }
    }
    
    
        
    
}

