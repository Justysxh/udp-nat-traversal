# udp-nat-traversal UDP实现的NAT穿越
udp-nat-traversal  用UDP实现的NAT穿越,即P2P穿透通信 理论上来说, 只要不是Port Restricted Cone NAT与Symmetric NAT , Symmetric NAT与Symmetric NAT.  这两种类型组合之间打洞, 都应该是可打通的. 具体原因请看原理.
(注: 以下均未考虑一机多IP的情况, 一般的移动设备或者电脑默认都同时只会有一张网卡工作,仅有一个IP)
## 原理
### NAT分类

#### **Full Cone NAT**:  
&ensp;&ensp;&ensp;&ensp;内网主机建立一个UDP socket(LocalIP:LocalPort) 第一次使用这个socket给外部主机发送数据时NAT会给其分配一个公网(PublicIP,PublicPort),以后用这个socket向外面**任何主机**发送数据都将使用这对(PublicIP,PublicPort)。此外**任何外部主机**只要知道这个(PublicIP,PublicPort)就可以发送数据给(PublicIP,PublicPort)，内网的主机就能收到这个数据包 
   
#### **Restricted Cone NAT**: 
&ensp;&ensp;&ensp;&ensp;内网主机建立一个UDP socket(LocalIP,LocalPort) 第一次使用这个socket给外部主机发送数据时NAT会给其分配一个公网(PublicIP,PublicPort),以后用这个socket向外面**任何主机**发送数据都将使用这对(PublicIP,PublicPort)。此外，如果任何外部主机想要发送数据给这个内网主机，只要知道这个(PublicIP,PublicPort)并且内网主机之前用这个**socket曾向这个外部主机IP发送过数据**。只要满足这两个条件，这个外部主机就可以用自己的(**IP,任何端口**)发送数据给(PublicIP,PublicPort)，内网的主机就能收到这个数据包 
   
#### **Port Restricted Cone NAT**:

&ensp;&ensp;&ensp;&ensp;内网主机建立一个UDP socket(LocalIP,LocalPort) 第一次使用这个socket给外部主机发送数据时NAT会给其分配一个公网(PublicIP,PublicPort),以后用这个socket向外面**任何主机**发送数据都将使用这对(PublicIP,PublicPort)。此外，如果任何外部主机想要发送数据给这个内网主机，只要知道这个(PublicIP,PublicPort)并且内网主机之前用这个**socket曾向这个外部主机(IP,Port)发送过数据**。只要满足这两个条件，这个外部主机就可以用自己的(**IP,Port**)发送数据给(PublicIP,PublicPort)，内网的主机就能收到这个数据包 
    
    
#### **Symmetric NAT**: 
&ensp;&ensp;&ensp;&ensp;内网主机建立一个UDP socket(LocalIP,LocalPort),当用这个socket第一次发数据给外部主机1时,NAT为其映射一个(PublicIP-1,Port-1),以后内网主机发送给外部主机1的所有数据都是用这个(PublicIP-1,Port-1)，如果内网主机同时用这个socket给外部主机2发送数据，第一次发送时，NAT会为其分配一个(PublicIP-2,Port-2), 以后内网主机发送给外部主机2的所有数据都是用这个(PublicIP-2,Port-2).如果NAT有多于一个公网IP，则PublicIP-1和PublicIP-2可能不同，如果NAT只有一个公网IP,则Port-1和Port-2肯定不同，也就是说一定不能是PublicIP-1等于 PublicIP-2且Port-1等于Port-2。此外，如果任何外部主机想要发送数据给这个内网主机，那么它首先应该收到内网主机发给他的数据，然后才能往回发送，否则即使他知道内网主机的一个(PublicIP,Port)也不能发送数据给内网主机，这种NAT无法实现UDP-P2P通信。

&ensp;&ensp;&ensp;&ensp;==同一个socket向不同外部主机通信,会分配不同的IP和端口, 只有对应的目标主机IP和端口才能与之通信,非常严格==


### 思路

&ensp;&ensp;&ensp;&ensp;既然已经知道了各种NAT类型的特点了, 也就可以知道,那三个cone类型的NAT, 同一个socket向外部的任何主机通信, NAT都会为它映射同一个端口,在外部主机看来, 就好像有固定的IP和端口一样.

&ensp;&ensp;&ensp;&ensp;即然对外面所有主机来说, 它的IP和端口一样, 那么我们做NAT穿透是不是就差最后一步了, 如何知道对方的IP和端口.

&ensp;&ensp;&ensp;&ensp;答案就是辅助服务器. 搭建一个服务器, 它有固定的外网IP和端口. 可以让所有的客户端都能连接它. 这样, 这个服务器就能知道所有连入它的客户端的外网IP和端口号了.

&ensp;&ensp;&ensp;&ensp;到这里思路就清楚了:

&ensp;&ensp;&ensp;&ensp;所有客户端都去连接辅助服务器,服务器就知道了所有客户端的外网ip和端口, 客户端再向服务器请求要穿透的目标客户端, 服务器就可以返回其目标的外网IP和端口, 同时通知目标客户端要被P2P连接并发送要连接它的另一个客户端的IP和端口. 这时双方都知道对方IP和端口,P2P就能顺利进行了.

&ensp;&ensp;&ensp;&ensp;细心的朋友可能发现了一个问题, 如何告诉辅助服务器,我需要哪个客户端的IP和端口呢?  答案是唯一标识符. 这里就可以有很多设计方案了, 比如事先两个客户端之间就协定好了这个标识符, 比如一个友好的用户名.

### 到这里就结束了吗?

&ensp;&ensp;&ensp;&ensp;显然不能, 其实还有一种情况未解决, 那就是,如果两个客户端,其中一个在Symmetric NAT后, 另一个在Cone NAT (F/R类型)后. 其实它们也是可以打通,实现P2P的.

比如客户端A在Symmetric NAT后, B在Cone NAT (F/R类型)后.
那么A到服务器与A到B的IP和端口都是不同的, 但B的外网IP和端口却是不变的. 这时B连接A肯定是不能通的, 但是A连接B却是可以的.
这时, 通过UDP通信中的recvfrom中的地址结构体, 就可以知道A的外网IP和端口了.  这时它们之间就可以实现P2P了.

(注:  F类型指Full Cone NAT;  R类型指 Restricted Cone NAT)

## 代码实现

### 辅助服务器实现(linux)

&ensp;&ensp;&ensp;&ensp;这里只是最简单的辅助服务器实现,仅仅发现有两个客户端连接,就向对方送IP和端口. 没有标识等等.

```

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
    
    sockaddr_in clientaddr[2]={0};//保存两个客户端的外网地址
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
        if(clients==2)//如果发现有两个客户端连接,就向对方发送另一个的IP和端口还有当前客户端自己的外网IP和端口(为确定两个客户端是否在同一个NAT后面)
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
```

### 客户端代码(linux)

&ensp;&ensp;&ensp;&ensp;简单实现, 向辅助服务器发送一个UDP包, 然后等待服务器返回另一个客户端的IP和端口, 然后直接向另一个客户端发送UDP包, 然后尝试接收数据, 如果接收失败,则再次发送, 多尝试几次. 一但接收成功, 再次向另一个客户端发一个UDP包, 最后一次发送,需要使用接收成功时的IP和端口,就是解决最严格的那种NAT.

```

int udpHoleClient()
{

    SOCKET  sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock == INVALID_SOCKET)
    {
        printf("create socket failed\n");
        return 0;
    }


    SOCKADDR_IN  myaddr = {0};
    myaddr.sin_port = htons(rand() % 800 + 9001);
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = INADDR_ANY;
    int ret = 0;

    int val = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)val, sizeof(val));
    //本地开启 UDP服务器,接收数据包
    ret = bind(sock, (SOCKADDR*)&myaddr, sizeof(SOCKADDR_IN));
    if(ret == -1)
    {
        printf("bind failed  error:%d\n", errno);
        close(sock);
        return 0;
    }
    SOCKADDR_IN servAddr = {0};
    servAddr.sin_port = htons(8888);
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr("123.147.223.222");
    //servAddr.sin_addr.S_un.S_addr = inet_addr("192.168.1.20");
    char buf[0x20] = {0};

    //向辅助服务器发送一个UDP包
    ret = sendto(sock, buf, 0x10, 0, (SOCKADDR*)&servAddr, sizeof(SOCKADDR_IN));

    SOCKADDR_IN recvAddr = {0};
    socklen_t addrLen = sizeof(SOCKADDR_IN);
    printf("wait recv peer addr...\n");
    //接收服务器返回的对方IP和端口, 还有自己的外见网IP和端口
    ret = recvfrom(sock, buf, 12, 0, (SOCKADDR*)&recvAddr, &addrLen);
    if(ret == -1)
    {
        printf("recv failed  error:%d\n",errno);
        close(sock);
        return 0;
    }

    printf("recv from: %s:%d\n", inet_ntoa(recvAddr.sin_addr), ntohs(recvAddr.sin_port));

    SOCKADDR_IN  peerAddr = {0};
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_addr.s_addr = *(int*)buf;
   // peerAddr.sin_addr.s_addr = 0x7B93DF7B;
    peerAddr.sin_port = *(short*)&buf[4];
    
    struct in_addr selfIp={0};
    selfIp.s_addr = *(int*)&buf[6];
    short selfPort = *(short*)&buf[10];
     //判断一下目标客户端跟自己是不是在同一个NAT后, 有相同的外网IP,基本上说明在同一个NAT后,当然,不一定在同一个子网中,因为可能有多级路由器
    if(peerAddr.sin_addr.s_addr == selfIp.s_addr)
    {
        printf("no need NAT hole, you and peer in the back of the same NAT\n");
        close(sock);
        return 0;
    }

    printf("recv data: My( %s:%d ) peer( %s:%d ) \n",
	 inet_ntoa(selfIp), ntohs(selfPort),
	 inet_ntoa(peerAddr.sin_addr), ntohs(peerAddr.sin_port));
    struct timeval timeout = {0,300000};//300ms
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
    char msg[0x20] = "0123456";
    struct passwd *pwd = getpwuid(getuid());
    sprintf(msg,"%d:[%s] hello",getpid(),pwd->pw_name);
    printf("send msg to peer: %s \n",msg);
    printf("wait peer back...\n");
    for(int i = 0; i < 5 ; ++i)
    {
        //向对方发送UDP包
        ret = sendto(sock, msg, strlen(msg) + 1, 0, (SOCKADDR*)&peerAddr, sizeof(SOCKADDR_IN));
        addrLen = sizeof(SOCKADDR_IN);
        //等待接收对方发送的UDP包
	ret = recvfrom(sock,buf,0x20,0,(SOCKADDR*)&recvAddr, &addrLen);
	if(ret >=0)
	{
	    break;
	}
    }
    if(ret<1)
    {
	printf("udp hole  failed!! errno:%d",errno);
        close(sock);
        return 0;
    }
    printf("recv from: %s:%d\n", inet_ntoa(recvAddr.sin_addr), ntohs(recvAddr.sin_port));
    printf("data: %s\n", buf);
    //如果接收成功,再用前面recv成功的IP和端口发送一次UDP包(解决其中一个是Symmetric NAT的问题)
    ret = sendto(sock, msg, strlen(msg) + 1, 0, (SOCKADDR*)&recvAddr, sizeof(SOCKADDR_IN));
    sleep(2);
    close(sock);
    return 0;

}

```

解释一下需要向目标客户端循环发送多次数据的原因.

那是因为当第一次发送时, 如果双方都不是Full Cone NAT, 那么必然数据包都到不了对方, 这时如果就结束了打洞流程, 那么打洞就失败了. 因为有recvfrom的超时时间, 这样可以保证双方都已经向对方的IP和端口发送了数据,也就是在NAT上建立了端口映射了. 这时再次发送数据就能到达对方了.

![image](https://github.com/Justysxh/udp-nat-traversal/blob/master/ok.png)

## 致谢

在实现UDP打洞的过程中,参考了网上许多的代码,但由于时间关系, 没有一一记录其引用地址, 但还是需感谢各们朋友们的分享精神.
