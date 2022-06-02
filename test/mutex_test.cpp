#include <mutex>
#include <thread>
#include <iostream>
#include <unistd.h>
#include <stdio.h>
using namespace std;

// 次线程先获得锁，并执行一个很长的任务
// 次线程在执行任务过程中被杀死
// 主线程申请获取相应的锁，能否成功（次线程被杀死之后锁资源是否会被释放）
void case1(){

    printf("case1-------------------\n");

    mutex m;

    auto f=[&m]{
        printf("Acquiring a lock ...\n");
        lock_guard<mutex> l(m);
        printf("Get the lock\n");
        sleep(5);
    };

    auto t=thread(f);

    // 等待1s，让线程能够获取锁资源
    sleep(1);

    pthread_cancel(t.native_handle());
    if(t.joinable()){
        t.join();
    }

    {
        lock_guard<mutex> l(m);
    }

    printf("Quit\n");
}

// 主线程先上锁
// 次线程尝试获取锁资源
// 主线程中取消次线程，并等待其完成
// 次线程是否会因无法获取锁资源而无法被取消（造成死锁）
void case2(){
    
    //cout<<this_thread::get_id()<<endl;

    printf("case2-------------------\n");

    mutex m1;

    auto f=[&m1]{
        printf("Acquiring a lock ...\n");
        lock_guard<mutex> l(m1);
        printf("Get the lock\n");
    };

    lock_guard<mutex> l(m1);

    sleep(1);

    auto t=thread(f);
    sleep(1);
    pthread_cancel(t.native_handle());

    printf("cancel\n");
    
    if(t.joinable()){
        t.join();
    }

    printf("Quit\n");
}

int main(){
    //cout<<this_thread::get_id()<<endl;
    case1();
    case2();
    return 0;
}