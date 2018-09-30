// create on: 2018-09-03
// author: ianma
// description: basic_kv.h
//

#define SUCCESS 0
#define NOT_INIT 2
#define ERROR 1
#define ERROR_KEY -1

#ifndef BASIC_KV
#define BASIC_KB

#include <iostream>
#include <cstdint>
#include <vector>


using std::vector;

class BasicKV
{
public:
    BasicKV();
    ~BasicKV();
    int Init(const uint64_t& init_data_size);
    int OpenChecker();
    int CloseChecker();
    int UnitTest(const char* input_file);
    int UnitTestCheck(const char* input_file);
    int Find(const uint32_t& key, char*& res, uint32_t& len);
    int Push(const uint32_t& key, const char* data, const uint32_t& len);
    int TestPrint(uint32_t start_pos, uint32_t end_pos);

private:
    int InitMemory();
    int InitTable();
    int GetMemoryPos(const uint64_t& offset, char*& res);


public:
    bool ready_for_checker;
    bool ready_for_pusher;

private:
    // build map from index to mem
    // index
    int64_t pre_key;
    uint64_t* table_offset;
    uint32_t* table;
    uint32_t table_number;
    // mem
    uint64_t data_size;
    uint32_t mem_part_num;
    uint32_t part_size;
    uint32_t extra_size_for_random;
    char** mem;
    uint64_t mem_pos;
};


#endif
