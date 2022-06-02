#include <misc/timer.h>

#include <iostream>
#include <chrono>
#include <unistd.h>

using namespace std;

int main(){
    int64_t timeinerval=2000000;
    auto task1=[](){
        for(;;){
            sleep(1);
            cout<<"task1"<<endl;
        }
    };
    unique_ptr<repeatTimer> ot[2];
    ot[0]=unique_ptr<repeatTimer>(new repeatTimer(timeinerval,task1,nullptr));
    ot[0]->Run();
    sleep(10);
    ot[0].reset();

    
    return 0;
}