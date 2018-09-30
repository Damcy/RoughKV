## RoughKV
a very simple structure Key-Value implemention with high concurrency

一个简单的高并发Key-Value系统

## Background
有一批KV数据（Key为uint32_t类型，不连续，Value的大小不超过2KB），一共有30亿条，Value一共90GB左右，需要在接近100G的内存中，将上诉数据存下，并能准确写入和读出，满足高并发要求。

## Analyse
1. KV系统一般是使用hash map实现
2. 在当前背景下，直接使用STL中的hash结构存储，内存远远不够，需要自己设计数据结构
3. sizeof(uint64_t) * (2^32 - 1)B + 90GB 远远超过提供的内存
4. Key值不连续，需要构造良好的存储方式

## Design
1. 	使用一个类BasicKV来进行统一处理，维护申请的内存
2. 	对于value的处理：
	- 统一使用char类型进行存储。考虑到总共的value有个90G+，这个部分的内存是必不可少的（不考虑对value进行压缩的前提下）
	- 一次性申请全量内存的方案是不可行的，因此需要分段申请。设计每个段申请1024\*1024\*1024大小的内存
	- 因为value的长短不一致，从几B到2KB不等，因此每一个段需要设置一定量的缓冲，避免数据溢出，使用1024\*1024的缓冲区
	- 最终，根据需要的数据内存总大小，进行相应的分段，检索的使用uint64_t的偏置，可以使用除法确定属于哪一个段，使用mod计算从段中的哪一个位置开始
3.  对于key的处理：
    - key是uint32_t类型的值，因此在表示的时候可以直接利用一个uint32_t大小的空间，顺序表示每一个key对应的value在内存中的地址，并利用上下key的差，来确定value的大小
	- 使用uint64_t的数值存value，需要8 \* 2^32 B = 32GB大小的空间，是不可行的，需要进行分片操作，记录每一个片的起始内存地址（uint64_t），使得使得片中能够使用uint32_t类型的地址进行存储，只记录偏移即可。
	- 考虑到每一个value最大为2KB，至多可以存下2^21个key，为了保险，令每一个片存2^20个key
	- 因为是连续内存存储，该方案需要在插入的时候key是连续的，从小到大的
	- （更优化方案可以使用uint16_t进行偏置，但需要更多分片）

## Functions
1. int Init(const uint64_t& init_data_size)
> 初始化函数，接受参数为需要申请的data_size的大小
2.	int InitMemory() 
> 初始化value内存块
3.	int InitTable() 
> 初始化key的偏移表示
4.	int GetMemoryPos(const uint64_t& offset, char\*& res)
> 根据连续的内存地址，计算所属的段id和偏移
5.	int Find(const uint32_t& key, char\*& res, uint32_t& len)
> 查找key对应的值，返回起始指针于res中，value长度存于len
6.	int Push(const uint32_t& key, const char* data, const uint32_t& len) 
> 根据len插入key对应的value
7.	int OpenChecker() 
> 在插入结束后，需要调用该函数开启查询接口
8.	int CloseChecker() 
> 在查询结束需要变更为插入模式时，需要调用这个接口
9.	int UnitTest(const char* input_file) 
> 对于push的单测，输入参数为二进制文件地址
10.	int UnitTestCheck(const char* input_file) 
> 对于check的单测，输入参数和UnitTest一致，会自动检测写入内存的value和文件中的是否一致

## Input
接收二进制数据，读取顺序为key，value_length，value

## Run
```shell
g++ -g -std=c++11 main.cpp BasicKV.cpp util.cpp -o kv_test
./kv_test
```