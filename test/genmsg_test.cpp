#include <type/genmsg.h>

#include <iostream>
using namespace std;

int main(){
        auto b1=genDeadBroadcast(1,"AA","BB");
    auto b2=genDeadBroadcast(1,"CC","DD");

    ComBroadcast cbc;
    auto x=cbc.add_bs();
    x->ParseFromString(b1.SerializeAsString());
    auto y=cbc.add_bs();
    y->ParseFromString(b2.SerializeAsString());
    //cbc.add_bs();

    auto a=genComMsg(cbc);

    cout<<a.DebugString()<<endl;

    return 0;
}