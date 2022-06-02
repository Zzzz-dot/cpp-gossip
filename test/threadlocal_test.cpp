#include <iostream>
#include <thread>
#include <unistd.h>

using namespace std;

class A{
    public:
        A(int x_):x(x_){
            cout<<"init: "<<this_thread::get_id()<<endl;
        }
        ~A(){
            cout<<"destroy: "<<this_thread::get_id()<<endl;
        }
        void Out(){
            cout<<x<<endl;
        }
    private:
        int x;
};

thread_local shared_ptr<A> a;

int main(){
    auto f1=[]{
        a=make_shared<A>(1);
        sleep(1);
        a->Out();
    };

    auto f2=[]{
        a=make_shared<A>(2);
        a->Out();
    };

    thread t1(f1);
    thread t2(f2);
    t1.join();
    t2.join();
    return 0;
}