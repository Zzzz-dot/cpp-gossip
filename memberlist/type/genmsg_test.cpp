#include <iostream>
#include "genmsg.hpp"
using namespace std;

int main(){
    MessageData md=genPushPull(false);
    addPushNodeState(md,"name1","addr1",10,10,PushNodeState::StateAlive);
    cout<<md.DebugString()<<endl;
    addPushNodeState(md,"name2","addr2",20,20,PushNodeState::StateAlive);
    cout<<md.DebugString()<<endl;
    return 0;
}