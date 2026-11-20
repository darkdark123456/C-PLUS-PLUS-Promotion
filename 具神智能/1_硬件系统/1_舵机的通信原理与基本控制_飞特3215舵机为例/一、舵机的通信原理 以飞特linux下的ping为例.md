### 一、**舵机的通信原理 以飞特linux下的ping为例**

######           **注意！！！！：在控制伺服电机运动的时候需要单独的进行供电，否则可能会损坏电机的EPROM**.使用usb供电仅测试读取寄存器操作。

######           注：从一种型号的舵机的操作推广到大多数的舵机操作，操作的流程都是有迹可循的，只不过可能每个厂商定义的通信协议不同、数据帧不同、寄存器数量不同、寄存器的地址不同，但是功能很多都是相同的。例如可以设置舵机的转向弧度、运动速度、加速度等，所以原理是相同的，只要学会了一种舵机的控制，就可以用类比的思想在实际工作中操作其他型号的舵机，这个文档就是由此而产生的。

**1、该厂商定义舵机的ping数据帧格式及各字节含义**

![image-20261120194540486](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261120194540486.png)



2、舵机的通信过程

```cpp
/*
Ping指令测试,测试总线上相应ID舵机是否就绪,广播指令只适用于总线只有一个舵机情况
*/

#include <iostream>
#include "SCServo.h"

SMS_STS sms_sts;//使用当前舵机的串口管理类定义一个串口管理对象

int main(int argc, char **argv)
{
	if(argc<2){
        std::cout<<"argc error!"<<std::endl;
        return 0;
	}
	std::cout<<"serial:"<<argv[1]<<std::endl;
    if(!sms_sts.begin(1000000, argv[1])){
        std::cout<<"Failed to init sms/sts motor!"<<std::endl;
        return 0;
    }
	int ID = sms_sts.Ping(1);
	if(ID!=-1){
		std::cout<<"ID:"<<ID<<std::endl;
	}else{
		std::cout<<"Ping servo ID error!"<<std::endl;
	}
	sms_sts.end();
	return 1;
}

```



![image-20261118215948847](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261118215948847.png)



```cpp
//Ping指令，返回舵机ID，超时返回-1
int	SCS::Ping(u8 ID)
{
	//在发送前清除输入端队列的内容
    rFlushSCS();
    
    // 构造相应的数据帧到buf中 
    // 将数据写入发送缓冲区，等待发送，这里不包括校验和
	writeBuf(ID, 0, NULL, 0, INST_PING);
	
    // 将buf写入到对应的fd编号的串口中
    wFlushSCS();
    
    // 存储工作状态 占1字节
	u8Status = 0;

	/*tcflush 清的是“已经进入内核 tty 输入队列”的字节。 
	它在拿到 tty 锁的状态下把队列清空，所以“正在清”的那一刻不会插进新数据；
	但系统调用一返回，到达的字节立刻就会进队列——这个时间差可能只有几微 */
	
    // 等待0xff 0xff开头的数据包
	if(!checkHead()){

		u8Error = ERR_NO_REPLY;
		return -1;
	}
    
    // ping应答包只有4字节数据
	u8 bBuf[4];
	u8Error = 0;
    
    // 如果读取的不是4字节 
	if(readSCS(bBuf, 4)!=4){
		u8Error = ERR_NO_REPLY;
		return -1;
	}
    
    
	if(bBuf[0]!=ID && ID!=0xfe){
		u8Error = ERR_SLAVE_ID;
		return -1;
	}
    
    // 如果有效的数据长度不是2字节
    // ping应答包 只有工作状态 检验和2字节数据
	if(bBuf[1]!=2){
		u8Error = ERR_BUFF_LEN;
		return -1;
	}
    
    // 计算检验和 如果收到的和数据帧的检验和字段不匹配
    // 直接丢弃 无纠错功能
	u8 calSum = ~(bBuf[0]+bBuf[1]+bBuf[2]);
	
    if(calSum!=bBuf[3]){
		u8Error = ERR_CRC_CMP;
		return -1;			
	}
    
    // 工作状态
	u8Status = bBuf[2];
	return bBuf[0];
}
```

```cpp
void SCSerial::rFlushSCS()
{
	tcflush(fd, TCIFLUSH); //发送前清除输入队列
}
```

```cpp
oid SCS::writeBuf(u8 ID, u8 MemAddr, u8 *nDat, u8 nLen, u8 Fun)
{
	u8 msgLen = 2; //ping指令长度为2，其他指令长度为参数长+2
	u8 bBuf[6];
	u8 CheckSum = 0;
	bBuf[0] = 0xff;   // 2帧头
	bBuf[1] = 0xff;  
	bBuf[2] = ID;   // 舵机ID
	bBuf[4] = Fun; //  指令类型
	if(nDat){
		msgLen += nLen + 1;
		bBuf[3] = msgLen;
		bBuf[5] = MemAddr; // 不论是读指令还是写指令 读写的起始地址总是固定在下标为5的这一个字节
		writeSCS(bBuf, 6);
		
	}else{ // ping 指令会走这个逻辑
		bBuf[3] = msgLen;
		writeSCS(bBuf, 5);
	}
	CheckSum = ID + msgLen + Fun + MemAddr;
	u8 i = 0;
	if(nDat){
		for(i=0; i<nLen; i++){
			CheckSum += nDat[i];
		}
		writeSCS(nDat, nLen);
	}
	writeSCS(~CheckSum);
}
```

```cpp
int SCSerial::writeSCS(unsigned char *nDat, int nLen)
{
	while(nLen--){
		txBuf[txBufLen++] = *nDat++;
	}
	return txBufLen; //将数据写入发送缓冲区，等待发送，这里不包括校验和
}
```

```cpp
// 直到这里才进行真正的缓冲区写操作
void SCSerial::wFlushSCS()
{
	if(txBufLen){
		txBufLen = write(fd, txBuf, txBufLen);
		txBufLen = 0;
	}
}

```

```cpp
// 内核函数 不想看 全是宏 只知道具体功能 
// 往fd串口编号写 size 为 n 的 buf
// fd串口编号 buf 写缓冲区  n 缓冲区字节数
extern ssize_t write (int __fd, const void *__buf, size_t __n) __wur
    __attr_access ((__read_only__, 2, 3));

```

```cpp
// 舵机如何完整的接收到一个数据帧？从哪开始？从哪结束？这里提供了一种解决方式，它使用2字节的滑动窗口，如果连续收到两个0xff 0xff就认为是一个满足条件的数据帧,但是这样操作也可能带来判断错误.

//*checkHead 是事后对齐，不是事前保证。
//它能吃掉少量脏字节，但吃不准“脏字节里恰好含 FF”的情况。 

// 缓冲里是 00 12 34 FF FF 01 02 00 FB（3 个垃圾 + 正常帧）

// 缓冲里是 00 12 34 FF(上一个数据帧的结尾) FF FF 01 02 00 FB（4个垃圾 + 正常帧） 此时这个算法就会失效 读取到
//FF(上一个数据帧的结尾) FF FF 01 02 00 而不是
//FF FF 01 02 00 FB  
*/
int SCS::checkHead()
{
	u8 bDat;
	u8 bBuf[] = {0, 0};
	u8 Cnt = 0;
	while(1){

		// 如果读取一个字节超时，直接返回-1，代表检查失败
		if(!readSCS(&bDat, 1)){
			return 0;
		}
		bBuf[1] = bBuf[0];
		bBuf[0] = bDat;

		// 如果连续读到两个 0xFF，就认为找到了帧头，返回 1
		if(bBuf[0]==0xff && bBuf[1]==0xff){
			break;
		}
		Cnt++;


/*
Cnt 每跳过一个无效字节加 1，超过 10 就 return 0。算下来：
能容忍FF FF前面有多少个垃圾 不超过10个，如果超过10个 直接丢弃此帧，返回0

垃圾 ≤ 9 个：能救回来
垃圾 ≥ 10 个：读到第 11 个字节还没凑出 FF FF，直接放弃，返回 0
这个阈值是为了快速失败——线上全是噪声时，没必要一直读到超时，
早点返回让上层重试 */

/**垃圾末尾正好是 0xFF 时会误判
垃圾 00 11 FF + 真帧头 FF FF */

		if(Cnt>10){
			return 0;
		}
	}
	return 1;
}
```

```cpp

// 选中对应的fd编号串口 ，进行读取，这里使用了位示图来进行操作，真的很巧妙
nt SCSerial::readSCS(unsigned char *nDat, int nLen)
{	
    int fs_sel;
    /**
     * fd_set = 一个装 16 个 long 的数组，共 16 × 64 = 1024 个 bit
    ┌────────────┬────────────┬─────┬────────────┐
    │ fds_bits[0]│ fds_bits[1]│ ... │fds_bits[15]│
    │  64 bit    │  64 bit    │     │  64 bit    │
    └────────────┴────────────┴─────┴────────────┘
    bit 0~63      bit 64~127           bit 960~1023

    index0       index1                index15
        0~63         64~127                960~1023
        fd0          fd64                  fd960
        fd1          fd65                  fd961
        fd2          fd66                  fd962
        ...          ...                   ...
        fd63         fd127                 fd1023
    */
    fd_set fs_read; // 串口的位示图
	int rvLen = 0;  // 已经读取到的字节数
    struct timeval time;
    /**
     * 把 fs_read 指向的那个 fd_set 里的 16 个 long 全部写成 0，
     * 也就是 1024 个 bit 全清
     */
    FD_ZERO(&fs_read);
    /**
     * 把 fs_read 指向的那个 fd_set 里的第 fd 个 bit 置 1，
     * 也就是把串口的位示图里对应的 bit 置 1
     */
    FD_SET(fd,&fs_read);

    time.tv_sec = 0;
    time.tv_usec = IOTimeOut*1000;

    //使用select实现串口的多路通信
	while(1){


        //select + read 是如何进行串口多路通信的？




        // select (串口范围，读位示图，写位示图，异常位示图，超时)
        // select能根据串口范围和具体的位示图，判断哪些串口有数据可读，哪些串口可以写入数据，哪些串口有异常情况发生。

        // 返回可以读的串口数量，如果返回值大于0，说明有数据可读，1代表有一个串口可读，2代表有两个串口可读；
        // 如果返回值等于0，说明超时，没有数据可读；
        // 如果返回值小于0，说明出错。
		fs_sel = select(fd+1, &fs_read, NULL, NULL, &time);
		
        
        
        if(fs_sel){

            // fd 是串口的文件描述符，read函数会从串口中读取数据，
            // 并将读取到的数据存储到nDat缓冲区中，最多读取nLen字节。
            // rvLen 是已经读取到的字节数，nLen-rvLen 是还需要读取的字节数。
            // nDat+rvLen 是指向缓冲区中下一个可写入的位置的指针。
			rvLen += read(fd, nDat+rvLen, nLen-rvLen);
			//printf("nLen = %d rvLen = %d\n", nLen, rvLen);
			if(rvLen<nLen){
				continue;
			}else{
				return rvLen;
			}
		}else{
			//printf("serial read fd read return 0\n");
			return rvLen;
		}
	}
}
```

```cpp
//将位示图整体置0
define __FD_ZERO(s) \
  do {									      \
    unsigned int __i;							      \
    fd_set *__arr = (s);						      \
    for (__i = 0; __i < sizeof (fd_set) / sizeof (__fd_mask); ++__i)	      \
      __FDS_BITS (__arr)[__i] = 0;					      \
  } while (0)



```

```c
// 将d编号串口在位示图s中的位置1，表示选中d有效，进行数据发送
#define __FD_SET(d, s) \
  ((void) (__FDS_BITS (s)[__FD_ELT(d)] |= __FD_MASK(d)))

// 因为程序中是使用16*8B=16*64bit的位示图来选中要发送数据的串口
// 在发送数据前 需要在位示图中找到该串口
// 串口在哪一个long字节里面？在该long字节中的第几位？
// 使用的是下面的思想
// pos_1=fd/n; pos_2=fd%n;

// 串口编号d在哪一个long字节里面？
#undef __FD_ELT
#define	__FD_ELT(d) \
  __extension__								    \
  ({ long int __d = (d);						    \
     (__builtin_constant_p (__d)					    \
      ? (0 <= __d && __d < __FD_SETSIZE					    \
	 ? (__d / __NFDBITS)						    \
	 : __fdelt_warn (__d))						    \
      : __fdelt_chk (__d)); })

// 在该long字节中的第几位
#define	__FD_MASK(d)	((__fd_mask) (1UL << ((d) % __NFDBITS)))
```

```c
// 只有函数的声明，定义在内核函数do_select中
// 只知道用法，没有看具体实现



//fs_sel = select(fd+1, &fs_read, NULL, NULL, &time);


// select (串口范围[0,fd]保证fd是一定能找到的，读位示图，写位示图，异常位示图，超时)
        // select能根据串口范围和具体的位示图，判断哪些串口有数据可读，哪些串口可以写入数据，哪些串口有异常情况发生。

        // 返回可以读的串口数量，如果返回值大于0，说明有数据可读，1代表有一个串口可读，2代表有两个串口可读；
        // 如果返回值等于0，说明超时，没有数据可读；
        // 如果返回值小于0，说明出错。



//int __nfds, 串口范围[0,fd]保证fd编号的串口是一定能找到的
//fd_set *__restrict __readfds, 读位示图操作
//fd_set *__restrict __writefds, 写位示图操作
//fd_set *__restrict __exceptfds, 异常位示图
//struct timeval *__restrict __timeout 超时
#ifndef __USE_TIME64_REDIRECTS
extern int select (int __nfds, fd_set *__restrict __readfds,
		   fd_set *__restrict __writefds,
		   fd_set *__restrict __exceptfds,
		   struct timeval *__restrict __timeout);
#else
# ifdef __REDIRECT
extern int __REDIRECT (select,
                       (int __nfds, fd_set *__restrict __readfds,
                        fd_set *__restrict __writefds,
                        fd_set *__restrict __exceptfds,
                        struct timeval *__restrict __timeout),
                       __select64);
# else
#  define select __select64
# endif
#endif
```

```c
typedef struct{
    /* XPG4.2 requires this member name.  Otherwise avoid the name
       from the global namespace.  */
#ifdef __USE_XOPEN
    __fd_mask fds_bits[__FD_SETSIZE / __NFDBITS];
# define __FDS_BITS(set) ((set)->fds_bits)
#else
    __fd_mask __fds_bits[__FD_SETSIZE / __NFDBITS];
# define __FDS_BITS(set) ((set)->__fds_bits)
#endif
  } fd_set; //返回一个数组大小16x64bit=1024bit
  
  //索引大小0-15  单个8字节
  
  
```

