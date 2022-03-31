#include <iostream>
#include "../memberlist/msgtype.h"
using namespace std;
int main(){
    MsgHead mh(pingMsg);
    MsgHead* mh_;
    Ping p(1,"a","a",1,"a");
    mh_=&p;
    cout<<mh_->Head<<endl;
    return 0;
}