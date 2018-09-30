#include <stdio.h>  
 
#define LOG_DEBUG "DEBUG"  
#define LOG_TRACE "TRACE"  
#define LOG_ERROR "ERROR"  
#define LOG_INFO  "INFOR"  
#define LOG_CRIT  "CRTCL"  
 
#define LOG(level, format, ...) \
    do { \
        fprintf(stderr, "[%s|%s@%s,%d] " format "\n", \
            level, __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
    } while (0)  



unsigned int get_msec(void);

