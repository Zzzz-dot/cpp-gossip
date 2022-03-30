#include "../misc/safe_queue.h"
#include <list>
#include <thread>
#include <iostream>
#include <chrono>
using namespace std;
int main(){
    safe_queue<int,list<int>> sq;
    auto f1=[&sq]{
        for (int i=0;i<1000000;i++){
            sq.push(i);
        }
    };

    auto f2=[&sq]{
        for(int i=0;i<500000;i++){
            sq.pop();
        }
    };

    thread t1(f1);
    thread t2(f2);

    auto chrono::
    t1.join();
    t2.join();

    cout<<sq.size()<<endl;
    return 0;

}