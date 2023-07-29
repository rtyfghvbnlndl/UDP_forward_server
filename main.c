#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<netinet/in.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include <sys/time.h>
#include <pthread.h>

#define MAX_NO_TRANSMISSION_TIMES 10
#define BUF_LEN 5
#define MAX_PEER 5

struct peerData{
    struct sockaddr_in source_addr;
    socklen_t source_addr_len;
    
    unsigned long int tx_buf;
    int counter;
    unsigned char tag;//配对的凭据
    unsigned char unread;
};

typedef struct workInfo{
    int udp;
    struct peerData peer[MAX_PEER];
    int end;

    int isCached;
    struct sockaddr_in source_addr;
    socklen_t source_addr_len;
    unsigned long int rx_buf;

} workInfo;

int udp;
struct sockaddr_in sock_addr;
socklen_t source_addr_len;
struct timeval timeOut;

void *sendWorker(void *args){
    workInfo* i = (workInfo*)args;
    while(1){
        for(int n=0; n<i->end; n++){
            //发现未发送的数据
            if(i->peer[n].unread){
                for(int m=0; m<i->end; m++){
                    //找到相同tag的peer
                    if(i->peer[n].tag==i->peer[m].tag){
                        sendto(i->udp, i->peer[n].tx_buf, BUF_LEN,  0, (struct sockaddr*)&(i->peer[m].source_addr), i->peer[m].source_addr_len);
                    }
                }
            }
        }
    }
    return NULL;
}

void *recvWorker(void *args){
    struct sockaddr_in source_addr;
    socklen_t source_addr_len;

    unsigned long int rx_buf;
    workInfo* i = (workInfo*)args;

    int counter = 0;
    int err;
    while(1){
        err = recvfrom(i->udp, rx_buf, BUF_LEN, 0, (struct sockaddr *)&(source_addr), &(source_addr_len));
        if(err>0){
            //等待主线程取走上一次的数据
            while(i->isCached){
                continue;
            }

            i->source_addr = source_addr;
            i->source_addr_len = source_addr_len;
            i->rx_buf = rx_buf;
        
            //应该通知主线程开始遍历
            i->isCached = 1;
        }    
    }
    return NULL;
}

int main(){

    udp = socket(AF_INET, SOCK_DGRAM, 0);

    timeOut.tv_sec = 1;
    timeOut.tv_usec = 0;
    sock_addr.sin_addr.s_addr = inet_addr("10.0.4.9");
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(8267);

    
    if (setsockopt(udp, SOL_SOCKET, SO_RCVTIMEO, &timeOut, sizeof(timeOut)) < 0){
        printf("time out setting failed\n");
    }
    if (bind(udp, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) < 0){
        printf("bind failed\n");
    }

    workInfo info;
    info.isCached = 0;

    //此处开始两个worker
    pthread_t recv_t;
    pthread_t send_t;

    pthread_create(&recv_t, NULL, recvWorker, &info);
    pthread_create(&send_t, NULL, sendWorker, &info);
    pthread_detach(recv_t);
    pthread_detach(send_t);

    int isRegistered;
    char* buf;
    struct peerData* p;
    while(1){
        while(!info.isCached){
            continue;
        }

        isRegistered = 0;
        for(int n=0; n<(info.end); n++){
                //识别peer，存入目标tx_buf
            if(info.peer[n].source_addr.sin_addr.s_addr==info.source_addr.sin_addr.s_addr && info.peer[n].source_addr.sin_port==info.source_addr.sin_port){
                info.peer[n].tx_buf = info.rx_buf;
                info.peer[n].unread = 1;
                info.peer[n].counter = 0;

                isRegistered = 1;
            }
            if(info.peer[n].counter++>info.end*MAX_NO_TRANSMISSION_TIMES){
                //长时间无数据，清除peer信息，并把最后一个移到这里，end--
                info.peer[n] = info.peer[info.end-1];
                info.end--;
            }
        }
        if(!isRegistered){
            //初次连接，判断握手，分配peer
            if(info.end<MAX_PEER){
                buf = &(info.rx_buf);

                //请求连接信号是前3个字节为0xff，第四个字节作为tag
                if(buf[0]==255&&buf[1]==255&&buf[2]==255){
                    info.peer[info.end].tag=buf[3];

                    p = &(info.peer[info.end]);
                    p->source_addr=info.source_addr;
                    p->source_addr_len=source_addr_len;

                    p->counter=0;
                    p->tx_buf=0;
                    p->unread=0;

                    info.end++;
                }
            }
        }
        info.isCached = 0;
    }
    
    close(udp);
    return 0;
}




