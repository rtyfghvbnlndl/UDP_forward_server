### UDP 转发服务器（未测试
转发收到的数据报，给一个或多个目标地址（广播）。如果称上述过程中的地址在一个组中，可以支持多组不同地址同时转发。

### 未测试

+ linux下使用
+ 多线程
+ 依靠ip和端口号识别不同数据来源
+ socket、udp

#### 流程
1. 发送至少4个字节: 
    > 例如：地址一发出：0xff 0xff 0xff 0x23  
    > 前三个字节为固定值标识连接请求，第四个字节取任意值记为tag
2. 如果有至少两个地址发出的tag相同，会开始转发。
    > 例如：  
    > 地址二发出：0xff 0xff 0xff 0x23  
    > 地址三发出：0xff 0xff 0xff 0x23  
    > 之后  
    > 地址二发出的数据会被转发给 地址一和地址三 
3. 超过一定限制未收到某一地址的信息,它的记录会被删除，需要重新连接。