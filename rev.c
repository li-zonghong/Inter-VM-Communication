#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <stdbool.h>

#include<time.h>
#include <pthread.h>
#include "my_queue.h"
#define GET_MAP 1
#define REP_GET_MAP 2
#define PR_BUF 3
#define max_offset  33554432


#pragma  pack(1)



struct pr_buff_packet{
    unsigned char src;
    unsigned char op;
    unsigned char len;
    unsigned char p[packet_length-3];
};

//存放共享内存的队列的信息


unsigned char * get_buff_map(unsigned int filesize,unsigned int offset);
int get_id();
void askforid(unsigned int filesize);
void * send_hander();




const char *filename = "/dev/ivshmem0";
unsigned int my_filesize =  0x10000;

unsigned int count =0;
unsigned char testchar[64];


int main(int argc,char **argv)
{
    que_init();
    mmap_init();
    int result,i,t,num;
    int max_num=my_filesize/packet_length;//块数目

    struct packet_msg *rev_msg; //接收块内存
    struct pr_buff_packet *pbp;
    
    struct timeval time,t1,t2,t3,t4;

    rev_msg = txbuff[(my_id-1)/2].buff;
    for(i=0;i<max_num;i++)
       rev_msg[i].value=0;
    printf("my_id:%d\n",my_id);
    printf("my_offset:%d\n",txbuff[(my_id-1)/2].offset);

    //testchar[0]=my_id;
    //testchar[1]=3;
    //testchar[2]=61; 
    //testchar[3]=1+'0';
    //testchar[63]=0;

    pthread_t send_thread,rev_thread;

    result = pthread_create(&send_thread,NULL,send_hander,NULL);
    if (result= 0) {
        printf("发送线程创建失败");
        return 0;
    } 
      i=1;         		
    		while(1){
          for(i=0;i<max_num;i++){
    			if(rev_msg[i].value%2!=0){
    				printf("rev :%d,%d,%d\n", i,  rev_msg[i].value,rev_msg[i].op);
            rev_msg[i].value=0;  		
    			}
    		}
        }
    return 0;
}





unsigned char * get_buff_map(unsigned int filesize, unsigned int offset)
{
    int fd;

    if((fd = open(filename, O_RDWR|O_NONBLOCK,0644))<0)
    {
        printf("%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    unsigned char *map;
    if((  map = (char *)mmap(0, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd,offset)) <= 0)
    {
        printf("%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    close(fd);
    return map;
}




void mmap_init()
{
    //my_id = 3;
    //dest_id = 1;
    int i;
    i = (my_id-1)/2;
    txbuff[i].id = my_id;
    txbuff[i].filesize = my_filesize;
    txbuff[i].offset = my_filesize;
    txbuff[i].buff = get_buff_map (txbuff[i].filesize,txbuff[i].offset);

    i= (dest_id-1)/2;
    txbuff[i].id = dest_id;
    txbuff[i].filesize = my_filesize;
    txbuff[i].offset = 0;
    txbuff[i].buff = get_buff_map (txbuff[i].filesize,txbuff[i].offset);
          
   return ;
}

void * send_hander()
{
    while(1)
    {
        send_packet();
        sleep(5);
    }
}





