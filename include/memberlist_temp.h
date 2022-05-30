#ifndef _TEMP_
#define _TEMP_

#include <atomic>
class timer;
//#include <misc/timer.h>

using namespace std;
class memberlist{
    friend timer;
    atomic<uint32_t> numNodes;
    public:
    memberlist(uint32_t n);
};

#endif