#include <misc/broadcastQueue.hpp>

#include <list>
#include <thread>
#include <iostream>
#include <chrono>
using namespace std;

typedef struct msg{
    string m1;
    string m2;
    string m3;
    msg(string m1_,string m2_,string m3_){
        m1=m1_;
        m2=m2_;
        m3=m3_;
    }
}msg;
int main(){
    //线程安全性测试
    safe_queue<int> sq1;
    auto f1=[&sq1]{
        for (int i=0;i<1000000;i++){
            sq1.push(i);
        }
    };
    auto f2=[&sq1]{
        for(int i=0;i<500000;i++){
            sq1.pop();
        }
    };
    thread t1(f1);
    thread t2(f2);
    t1.join();
    t2.join();
    cout<<sq1.size()<<endl;

    //传递右值效率测试
    safe_queue<msg> sq2;
    safe_queue<msg> sq3;
    safe_queue<msg> sq4;

    auto m=msg("aaaaaaaaaaaaaaaaaaaaaa","bbbbbbbbbbbbbbbbbbbbbbbbb","cccccccccccccccc");

    auto f3=[&sq2,m]{
        for (int i=0;i<10000000;i++){
            sq2.push(m);
        }
    };

    auto f4=[&sq3]{
        for (int i=0;i<10000000;i++){
            sq3.push(msg("aaaaaaaaaaaaaaaaaaaaaa","bbbbbbbbbbbbbbbbbbbbbbbbb","cccccccccccccccc"));
        }
    };

    auto f5=[&sq4]{
        for (int i=0;i<10000000;i++){
            sq4.emplace("aaaaaaaaaaaaaaaaaaaaaa","bbbbbbbbbbbbbbbbbbbbbbbbb","cccccccccccccccc");
        }
    };
    //hread t3(f3);
    auto start=chrono::system_clock::now();
    //t3.join();
    auto end=chrono::system_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    cout<<duration.count()<<endl;

    thread t4(f4);
    start=chrono::system_clock::now();
    t4.join();
    end=chrono::system_clock::now();
    duration = chrono::duration_cast<chrono::microseconds>(end - start);
    cout<<duration.count()<<endl;

    //thread t5(f5);
    start=chrono::system_clock::now();
    //t5.join();
    end=chrono::system_clock::now();
    duration = chrono::duration_cast<chrono::microseconds>(end - start);
    cout<<duration.count()<<endl;

    return 0;

}