#include "../misc/timer.hpp"

#include <iostream>
#include <chrono>
#include <unistd.h>

using namespace std;

int main(){
    auto f=[](){
        cout<<1<<endl;
    };
    uint32_t timeint=1000;
    timer t(timeint,f);
    t.Run();
    sleep(15);
    t.Stop();
    return 0;
}