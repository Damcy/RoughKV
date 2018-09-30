#include <iostream>
#include <string>

#include "util.h"
#include "BasicKV.h"


int main(int argc, char* argv[])
{
    uint64_t data_size = 1024 * 1024;
    data_size *= (1024 * 90);
    BasicKV* obj = new BasicKV();
    if(obj->Init(data_size)) {
        LOG(LOG_ERROR, "init obj failed");
        delete obj;
        return 1;
    }
//    uint32_t key = 1;
//    const char* data = "Test";
//    uint32_t len = 4;
//    obj->Push(key, data, len);
    obj->UnitTest("/data/rextang/key_value.dat");
    //obj->OpenChecker();
    obj->UnitTestCheck("/data/rextang/key_value.dat");

    return 0;
}
