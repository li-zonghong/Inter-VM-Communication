#ifndef _MY_QUEUE_H_
#define _MY_QUEUE_H_
#define packet_length 64 //块大小@
#define MAX_MSG_NUM 100 //队列最大长度 
#define PRIORITY_NUM 3 //优先级数目
#define max_vm 2 //最大主机数
unsigned char my_id = 1;
unsigned dest_id =  3;

unsigned char state=0; //用来表示当前的发送队列状态
unsigned int send_num = 0;

struct mesg 
{
  unsigned char dest;           //目的id
  struct mesg *next;            
  unsigned char text[packet_length];    //发送内容
  unsigned int len;                 //长度
  unsigned char priority;           //优先级,优先级越小，优先级越高
  //static unsigned char num;
};
struct mesg *head[PRIORITY_NUM],*mqend[PRIORITY_NUM],*empty_que_head, *empty_que_tail;//队头和队尾

struct packet_msg{
    unsigned char value;
    //unsigned char dest;
    //unsigned ychar src;
    unsigned char op; 
    unsigned char p[packet_length-2];
};

struct RWBUFF 
{
  int id;
  struct packet_msg *buff;
  unsigned int filesize;
  unsigned int offset;
 }txbuff[max_vm];


int insert_mesg(struct mesg *msg);
int send_msg(unsigned char *p,unsigned int length,unsigned char priority,unsigned char  dest);
int send_packet(void);


#endif
