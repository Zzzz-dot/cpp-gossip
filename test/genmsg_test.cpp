#include <type/genmsg.h>

#include <iostream>
using namespace std;

int main(){

    MessageData a=genAlive(1,"123","123",1);
    auto as=a.SerializeAsString();
    MessageData d=genDead(1,"123","123");
    auto ds=d.SerializeAsString();
    auto cs=as+ds;
    cout<<cs<<endl;

    Compound cd=genCompound();
    cout<<cd.SerializeAsString()<<endl;
    cout<<cd.DebugString()<<endl;

    cd.ParseFromString(as);
    cout<<cd.SerializeAsString()<<endl;
    cout<<cd.DebugString()<<endl;


    cd.ParseFromString(cs);
    cout<<cd.SerializeAsString()<<endl;
    cout<<cd.DebugString()<<endl;

    cd.Clear();
    addMessage(cd,a);
    auto cds0=cd.SerializeAsString();
    addMessage(cd,d);
    auto cds1=cd.SerializeAsString();

    cd.ParseFromString(cds0+cds1);
    cout<<cd.SerializeAsString()<<endl;
    cout<<cd.DebugString()<<endl;

    auto x=cd.mutable_mds();
    cout<<cd.mds_size()<<endl;


    //信息以cd或者数量为1的cd发过来
    //

    // MessageData md;
    // md.ParseFromString(cs);
    // cout<<md.SerializeAsString()<<endl;
    // cout<<md.DebugString()<<endl;

    return 0;
}