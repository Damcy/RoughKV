// create on: 2018-09-03
// author: ianma
// description: basic_kv.h

#include <cstring>
#include <fstream>
#include <iostream>

#include "util.h"
#include "BasicKV.h"

// suppose max value size is 2KB(2^11), each table can contain no more than 2^(32-11)==2^21 keys, total size is le 2^32
// for safe, assign 2^20 keys for each table
#define TABLE_ID_OFFSET 20
#define TABLE_POS_FACTOR 0xFFFFF
#define MAX_VAL_SIZE 2048


BasicKV::BasicKV()
{
    ready_for_checker = false;
    ready_for_pusher = false;
    mem_pos = 0;
    pre_key = -1;
}


BasicKV::~BasicKV()
{
    //delete[] data_container;
    for (uint32_t i = 0; i < mem_part_num; ++i) {
        free(mem[i]);
    }
    free(mem);
    // table
    free(table_offset);
    free(table);
}


int BasicKV::OpenChecker()
{
    if (ready_for_pusher == false) {
        LOG(LOG_ERROR, "puhser should be ready before open checker");
        return ERROR;
    }
    ready_for_checker = true;
    ready_for_pusher = false;
    return ready_for_checker;
}


int BasicKV::CloseChecker()
{
    ready_for_checker = false;
    ready_for_pusher = true;
    return ready_for_pusher;
}


int BasicKV::InitMemory()
{
    // get large mem for record, split into parts, each part have 1GB space
    part_size = 1024 * 1024 * 1024;
    uint32_t extra_size_for_random = 1024 * 1024;
    // malloc data
    mem_part_num = (uint32_t)(data_size / part_size) + 1; 
    if ((mem = (char**) malloc(sizeof(char*) * mem_part_num)) == NULL) {
        LOG(LOG_ERROR, "malloc failed for mem!");
        return ERROR;
    }
    for (int i = 0; i < mem_part_num; ++i) {
        if ((mem[i] = (char*) malloc(sizeof(char) * (part_size + extra_size_for_random))) == NULL) {
            LOG(LOG_ERROR, "malloc failed! part index: %d", i);
            return ERROR;
        }
    }
    // end of malloc data
    // init mem start position
    mem_pos = 0;
    return SUCCESS;
}


int BasicKV::InitTable()
{
    table_number = 1 << (32 - TABLE_ID_OFFSET);
    if ((table_offset = (uint64_t*) malloc(sizeof(uint64_t) * table_number)) == NULL) {
        LOG(LOG_ERROR, "malloc failed for table_offset");
        return ERROR;
    } else {
        //memset(table_offset, 1, sizeof(uint64_t) * table_number);
        memset(table_offset, 0xFF, sizeof(uint64_t) * table_number);
    }
    if ((table = (uint32_t*) malloc(sizeof(uint32_t) * 0xFFFFFFFF)) == NULL) {
        LOG(LOG_ERROR, "malloc failed for table");
        return ERROR;
    } else {
        memset(table, 0xFF, sizeof(uint32_t) * 0xFFFFFFFF);
    }
//    } else {
//        for (uint32_t i = 0; i < table_number; ++i) {
//            if ((table[i] = (uint32_t*) malloc(sizeof(uint32_t) * TABLE_POS_FACTOR)) == NULL) {
//                LOG(LOG_ERROR, "malloc failed for table in index %d", i);
//                return ERROR;
//            } else {
//                memset(table[i], 0xFF, sizeof(uint32_t) * TABLE_POS_FACTOR);
//            }
//        }
//    }
    table_offset[0] = 0;
    return SUCCESS;
}


int BasicKV::GetMemoryPos(const uint64_t& offset, char*& res)
{
    uint32_t part_index = offset / part_size;
    uint32_t part_offset = offset % part_size;
    // get char* from PART #part_index and OFFSET #part_offset
    res = mem[part_index] + part_offset;
    return SUCCESS;
}


int BasicKV::Init(const uint64_t& init_data_size)
{
    data_size = init_data_size;
    // init mem
    if (InitMemory()) {
        LOG(LOG_ERROR, "InitMemory failed.");
        return ERROR;
    }
    LOG(LOG_INFO, "InitMemory success.");
    // init table for index
    if (InitTable()) {
        LOG(LOG_ERROR, "InitTable failed.");
        return ERROR;
    }
    LOG(LOG_INFO, "InitTable success.");
    ready_for_pusher = true;
    // trick
    ready_for_checker = true;
    return SUCCESS;
}


int BasicKV::Push(const uint32_t& key, const char* data, const uint32_t& len)
{
    if (key > 0 && pre_key >= key) {
        LOG(LOG_ERROR, "error push key, key should larger than pre push key, key: %u, last key, %ld", key, pre_key);
        return ERROR;
    }
    pre_key = key;
    uint32_t table_id = key >> TABLE_ID_OFFSET;
    //uint32_t table_pos = key & TABLE_POS_FACTOR;
    // check if the first key for the table
    if (table_offset[table_id] == 0xFFFFFFFFFFFFFFFF && table_id > 0) {
        table_offset[table_id] = mem_pos;
    }
    // local offset
    uint32_t local_offset = mem_pos - table_offset[table_id] + len;
    //table[table_id][table_pos] = local_offset;
    table[key] = local_offset;
    // get write pos
    char* write_pos = NULL;
    GetMemoryPos(mem_pos, write_pos);
    // copy data into mem
    memcpy(write_pos, data, len);
    mem_pos += len;
    return SUCCESS;
}


int BasicKV::Find(const uint32_t& key, char*& res, uint32_t& len)
{
    uint32_t table_id = key >> TABLE_ID_OFFSET;
    uint32_t table_pos = key & TABLE_POS_FACTOR;
    int32_t reduce_pos = table_pos - 1;
    uint32_t reduce_key = key - 1;
    if (table[key] == 0xFFFFFFFF) {
        return ERROR_KEY;
    }
    while (table[reduce_key] == 0xFFFFFFFF && reduce_pos >= 0) {
        --reduce_pos;
        --reduce_key;
    }
    uint32_t last_key_end = (reduce_pos < 0) ? 0 : table[reduce_key];
    len = table[key] - last_key_end;
    uint64_t mem_start = table_offset[table_id] + last_key_end;
    GetMemoryPos(mem_start, res);
    return SUCCESS;
}


int BasicKV::UnitTestCheck(const char* input_file)
{
    if (ready_for_checker == false) {
        LOG(LOG_ERROR, "please init the obj first by calling Init()");
        return NOT_INIT;
    }
    LOG(LOG_INFO, "start checking");
    FILE* fin = fopen(input_file, "r");
    if (fin == NULL) {
        LOG(LOG_ERROR, "can not open data file: %s", input_file);
        return ERROR;
    }
    uint32_t cnt = 0;
    LOG(LOG_INFO, "start reading file");
    uint32_t start = get_msec();
    while (true) {
        uint32_t key = 0;
        if (fread(&key, sizeof(key), 1, fin) == 0) {
            // end of file
            break;
        }
        uint32_t val_size = 0;
        if (fread(&val_size, sizeof(val_size), 1, fin) == 0) {
            // end of file
            break;
        }
        char buffer[MAX_VAL_SIZE];
        if (fread(buffer, sizeof(char), val_size, fin) == 0) {
            LOG(LOG_ERROR, "read data failed");
            fclose(fin);
            return ERROR;
        }
        char* kv_data = NULL;
        uint32_t kv_size;
        if (Find(key, kv_data, kv_size)) {
            LOG(LOG_ERROR, "failed with key: %u", key);
            break;
        } else {
            if (val_size != kv_size || strncmp(kv_data, buffer, val_size)) {
                LOG(LOG_ERROR, "not equal, key: %u, val_size: %u, kv_size: %u", key, val_size, kv_size);
                TestPrint(key - 30, key + 1);
                break;
            }
        }
        ++cnt;
    }
    fclose(fin);
    LOG(LOG_INFO, "spent time: %d ms", get_msec());
    // finish push
    LOG(LOG_INFO, "finish check data");

    return SUCCESS;
}


int BasicKV::UnitTest(const char* input_file)
{
    if (ready_for_pusher == false) {
        LOG(LOG_ERROR, "please init the obj first by calling Init()");
        return NOT_INIT;
    }
    // open data file
    //ifstream ifs(input_file, ios::binary);
    FILE* fin = fopen(input_file, "r");
    if (fin == NULL) {
        LOG(LOG_ERROR, "can not open data file: %s", input_file);
        return ERROR;
    }
    uint32_t cnt = 0;
    uint32_t max_key = 0;
    LOG(LOG_INFO, "start reading file");
    uint32_t start = get_msec();
    while (true) {
        uint32_t key = 0;
        if (fread(&key, sizeof(key), 1, fin) == 0) {
            // end of file
            break;
        }
        uint32_t val_size = 0;
        if (fread(&val_size, sizeof(val_size), 1, fin) == 0) {
            // end of file
            break;
        }
        char buffer[MAX_VAL_SIZE];
        if (fread(buffer, sizeof(char), val_size, fin) == 0) {
            LOG(LOG_ERROR, "read data failed");
            fclose(fin);
            return ERROR;
        }
        if (Push(key, buffer, val_size) != 0) {
            LOG(LOG_ERROR, "push data failed, key: %d, val: %s, len: %d", key, buffer, val_size);
            fclose(fin);
            return ERROR;
        }
//        if (cnt % 30 == 0) {
//            LOG(LOG_INFO, "key: %d, val_size: %d, val: %s", key, val_size, buffer);
//            TestPrint(cnt - 30, cnt);
//            break;
//        }
        ++cnt;
        max_key = key;
    }
    fclose(fin);
    LOG(LOG_INFO, "push cnt: %u", cnt);
    LOG(LOG_INFO, "max key: %u", max_key);
    LOG(LOG_INFO, "spent time: %d ms", get_msec());
    // finish push
    LOG(LOG_INFO, "finish push data");

    return SUCCESS;
}


int BasicKV::TestPrint(uint32_t start_pos, uint32_t end_pos)
{
    LOG(LOG_INFO, "-----------------------------");
    LOG(LOG_INFO, "pre key: %ld", pre_key);
    LOG(LOG_INFO, "--------table info-----------");
    LOG(LOG_INFO, "table number: %u", table_number);
    for (uint32_t i = 0; i < table_number; ++i) {
        if (i % 1000 == 0 || i % 1001 == 0) {
            LOG(LOG_INFO, "table, id: %u, offset: %lu", i, table_offset[i]);
        }
    }
    for (uint32_t i = start_pos; i < end_pos; ++i) {
        LOG(LOG_INFO, "second offset, key: %u, offset: %u", i, table[i]);
    }
    return SUCCESS;
}

