#ifndef _MY_QUEUE_H_
#define _MY_QUEUE_H_
#define packet_length 64 //块大小@
#define MAX_MSG_NUM 100 //队列最大长度 
#define PRIORITY_NUM 8 //优先级数目
#define max_vm 2 //最大主机数
unsigned char my_id = 1;
unsigned dest_id =  3;

unsigned char state=0; //用来表示当前的发送队列状态
unsigned int send_num = 0;

struct mesg     //队列节点
{
  unsigned char dest;           //目的id
  struct mesg *next;            
  unsigned char text[packet_length];    //发送内容
  unsigned int len;                 //长度
  unsigned char priority;           //优先级,优先级越小，优先级越高
  //static unsigned char num;
};
struct mesg *head[PRIORITY_NUM],*mqend[PRIORITY_NUM],*empty_que_head, *empty_que_tail;//队头和队尾

struct packet_msg{  //数据包的组成
    unsigned char value;    //状态字节
    unsigned char op;       //操作数
    unsigned char p[packet_length-2];   //数据字段
};

struct RWBUFF //接收内存
{
  int id;   
  struct packet_msg *buff; 
  unsigned int filesize;
  unsigned int offset;
 }txbuff[max_vm];


int insert_mesg(struct mesg *msg); 
int send_msg(unsigned char *p,unsigned int length,unsigned char priority,unsigned char  dest);
int send_packet();


//对应于分拣调度模块，发送函数所实际调用的函数
int send_msg(unsigned char *p,unsigned int length,unsigned char priority,unsigned char  dest)
{   
    struct  mesg *msg;
    int i,n,j;
    //if(msg == NULL)return -1; 
    //发送部分
    msg = empty_que_head;
    i = (dest-1)/2;
    n= txbuff[i].filesize/packet_length;
    switch (priority)
    {
    case 0:
        if((state&0x01)==0){
            state= (state|0x01);
            for(int j=0;j<n;j++)
            {
                if(txbuff[i].buff[j].value==0)
                {
                memcpy((unsigned char*)&txbuff[i].buff[j],p,length);
                printf("send_msg:ok,%d\n",j);
                txbuff[i].buff[j].value =my_id;
                if((state|0x01)==0x01&&head[0]->next==NULL)state=(state^0x01);
                return 1;
                }
            }   
        }
        if(empty_que_head->next==NULL) return -1;   //队列已满
        empty_que_head = empty_que_head->next; 
        msg->next =NULL;
        msg->len = length;
        msg->dest = dest;
        msg->priority =priority;
        memcpy(msg->text, p,length);
        state= (state|0x01);
        if(head[0]->next==NULL){mqend[0] = msg; head[0]->next=msg;}
        else {mqend[0]->next = msg; mqend[0] = mqend[0]->next;}
        return 0;
        break;
    case 1:
        if((state&0x03)==0){
            state= (state|0x02);
            for(int j=0;j<n;j++)
            {
                if(txbuff[i].buff[j].value==0)
                {
                memcpy((unsigned char*)&txbuff[i].buff[j],p,length);
                printf("send_msg:ok,%d\n",j);
                txbuff[i].buff[j].value =my_id;
                if((state|0x02)==0x02&&head[1]->next==NULL)state=(state^0x02);
                return 1;
                }
            }   
        }
        if(empty_que_head->next==NULL) return -1;
        empty_que_head = empty_que_head->next; 
        msg->next =NULL;
        msg->len = length;
        msg->dest = dest;
        msg->priority =priority;
        memcpy(msg->text, p,length);
        state= (state|0x02);
        if(head[1]->next==NULL){mqend[1] = msg; head[1]->next=msg;}
        else {mqend[1]->next = msg; mqend[1] = mqend[1]->next;}
        break;
    case 2:
        if((state&0x07)==0){
           state= (state|0x04);
           j=send_num;
             while(1){
             	j++;
            	if(j>=n)j=0;
                if(txbuff[i].buff[j].value==0)
                {
                memcpy((unsigned char*)&txbuff[i].buff[j],p,length);
                printf("send_msg:ok,%d\n",j);
                txbuff[i].buff[j].value =my_id;
                if(head[2]->next==NULL)state=(state&(~0x04));
                send_num=j;
                return 1;
                }
            }
           printf("%d\n",j);
        }
        if(empty_que_head->next==NULL) return -1;
        empty_que_head = empty_que_head->next; 
        msg->next =NULL;
        msg->len = length;
        msg->dest = dest;
        msg->priority =priority;
        memcpy(msg->text, p,length);
        state= (state|0x04);
        if(head[2]->next==NULL){mqend[2] = msg; head[2]->next=msg;}
        else {mqend[2]->next = msg; mqend[2] = mqend[2]->next;}
        return 0;
        break;
    case 3:
        if((state&0x0f)==0){
            state= (state|0x08);
            for(int j=0;j<n;j++)
            {
                if(txbuff[i].buff[j].value==0)
                {
                memcpy((unsigned char*)&txbuff[i].buff[j],p,length);
                txbuff[i].buff[j].value =my_id;
                if((state|0x08)==0x08&&head[3]->next==NULL)state=(state^0x08);
                return 1;
                }
            }   
        }
        if(empty_que_head->next==NULL) return -1;
        empty_que_head = empty_que_head->next; 
        msg->next =NULL;
        msg->len = length;
        msg->dest = dest;
        msg->priority =priority;
        memcpy(msg->text, p,length);
        state= (state|0x08);
        if(head[3]->next==NULL){mqend[3] = msg; head[3]->next=msg;}
        else {mqend[3]->next = msg; mqend[3] = mqend[3]->next;}
        break;
    case 4:
        if((state&0x1f)==0){
           state= (state|0x10);
           j=send_num;
             while(1){
             	j++;
            	if(j>=n)j=0;
                if(txbuff[i].buff[j].value==0)
                {
                memcpy((unsigned char*)&txbuff[i].buff[j],p,length);
                printf("send_msg:ok,%d\n",j);
                txbuff[i].buff[j].value =my_id;
                if(head[4]->next==NULL)state=(state&(~0x10));
                send_num=j;
                return 1;
                }
            }
           printf("%d\n",j);
        }
        if(empty_que_head->next==NULL) return -1;
        empty_que_head = empty_que_head->next; 
        msg->next =NULL;
        msg->len = length;
        msg->dest = dest;
        msg->priority =priority;
        memcpy(msg->text, p,length);
        state= (state|0x10);
        if(head[4]->next==NULL){mqend[4] = msg; head[4]->next=msg;}
        else {mqend[4]->next = msg; mqend[4] = mqend[4]->next;}
        return 0;
        break;
    case 5:
        if((state&0x3f)==0){
           state= (state|0x20);
           j=send_num;
             while(1){
             	j++;
            	if(j>=n)j=0;
                if(txbuff[i].buff[j].value==0)
                {
                memcpy((unsigned char*)&txbuff[i].buff[j],p,length);
                printf("send_msg:ok,%d\n",j);
                txbuff[i].buff[j].value =my_id;
                if(head[5]->next==NULL)state=(state&(~0x20));
                send_num=j;
                return 1;
                }
            }
           printf("%d\n",j);
        }
        if(empty_que_head->next==NULL) return -1;
        empty_que_head = empty_que_head->next; 
        msg->next =NULL;
        msg->len = length;
        msg->dest = dest;
        msg->priority =priority;
        memcpy(msg->text, p,length);
        state= (state|0x20);
        if(head[5]->next==NULL){mqend[5] = msg; head[5]->next=msg;}
        else {mqend[5]->next = msg; mqend[5] = mqend[5]->next;}
        return 0;
        break;
    case 6:
        if((state&0x7f)==0){
           state= (state|0x40);
           j=send_num;
             while(1){
             	j++;
            	if(j>=n)j=0;
                if(txbuff[i].buff[j].value==0)
                {
                memcpy((unsigned char*)&txbuff[i].buff[j],p,length);
                printf("send_msg:ok,%d\n",j);
                txbuff[i].buff[j].value =my_id;
                if(head[6]->next==NULL)state=(state&(~0x40));
                send_num=j;
                return 1;
                }
            }
           printf("%d\n",j);
        }
        if(empty_que_head->next==NULL) return -1;
        empty_que_head = empty_que_head->next; 
        msg->next =NULL;
        msg->len = length;
        msg->dest = dest;
        msg->priority =priority;
        memcpy(msg->text, p,length);
        state= (state|0x40);
        if(head[6]->next==NULL){mqend[6] = msg; head[6]->next=msg;}
        else {mqend[6]->next = msg; mqend[6] = mqend[6]->next;}
        return 0;
        break;
    case 7:
        if((state&0x7f)==0){
           state= (state|0x80);
           j=send_num;
             while(1){
             	j++;
            	if(j>=n)j=0;
                if(txbuff[i].buff[j].value==0)
                {
                memcpy((unsigned char*)&txbuff[i].buff[j],p,length);
                printf("send_msg:ok,%d\n",j);
                txbuff[i].buff[j].value =my_id;
                if(head[7]->next==NULL)state=(state&(~0x80));
                send_num=j;
                return 1;
                }
            }
           printf("%d\n",j);
        }
        if(empty_que_head->next==NULL) return -1;
        empty_que_head = empty_que_head->next; 
        msg->next =NULL;
        msg->len = length;
        msg->dest = dest;
        msg->priority =priority;
        memcpy(msg->text, p,length);
        state= (state|0x80);
        if(head[7]->next==NULL){mqend[7] = msg; head[7]->next=msg;}
        else {mqend[7]->next = msg; mqend[7] = mqend[7]->next;}
        return 0;
        break;
    default:
        break;
    }
    return 0;
}


//对应于发送引擎，每次从发送队列中取出一个数据包发送
int send_packet()
{
    struct mesg *tmp;
    char *p;
    unsigned int i,k;
    unsigned int t;
    for(t=0;t<PRIORITY_NUM;t++)
    {
        if(head[t]->next!=NULL) break;
    }
    if(t==PRIORITY_NUM){state=0;//printf("NULL\n");
    return -1;}
    tmp=head[t]->next;
    i=(tmp->dest-1)/2;
    int n;
    n= txbuff[i].filesize/packet_length;
    for(int j=0;j<n;j++)
    {
        if(txbuff[i].buff[j].value==0)
        {
            //printf("\n");
            if(tmp->len == 0){  printf("error:length is zero\n");return 0;}
            txbuff[i].buff[j].value=my_id+1;
            memcpy((unsigned char*)&txbuff[i].buff[j],tmp->text,tmp->len);
            printf("send_packet:ok,%d\n",j);
            txbuff[i].buff[j].value = my_id;
            if(head[t]->next->next==NULL){mqend[t]=head[t]->next;}
            head[t]->next=head[t]->next->next;
            tmp->next==NULL;
            empty_que_tail->next=tmp;
            empty_que_tail = tmp;
            return j;
        }
    }
    return -1;
}

void que_init()
{
    struct  mesg *tmp;
    int i;
    state=0;
    empty_que_head = (struct mesg *)malloc(sizeof(struct  mesg));
    tmp =empty_que_head;
    for(i=0;i<MAX_MSG_NUM;i++)
    {
        tmp->next = (struct mesg *)malloc(sizeof(struct  mesg));
        tmp = tmp->next;
    }
     empty_que_tail = tmp;
     if(empty_que_tail==NULL){ printf("error");}
   for(i=0;i<PRIORITY_NUM;i++)
    {
        head[i]=(struct mesg *)malloc(sizeof(struct  mesg));
        head[i]->len = 0;
        //head[i]->text = NULL;
        head[i]->priority = 7;
        mqend[i]=head[i];
         //mqend[i]=NULL;
         //head[i]=NULL;
    }
    return;
}

#endif
