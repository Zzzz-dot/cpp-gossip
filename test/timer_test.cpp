#include <misc/timer.h>

#include <iostream>
#include <chrono>
#include <unistd.h>

using namespace std;

int main(){
    int64_t timeinerval=2000000;
    auto task1=[](){
        cout<<"task1"<<endl;
    };
    auto ot=make_shared<onceTimer>(timeinerval,task1,nullptr);

    
    return 0;
}