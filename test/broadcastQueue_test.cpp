#include <misc/broadcastQueue.h>

using namespace std;

int main(){
    auto f=[]()->uint32_t{
        return 1;
    };

    broadcastQueue bq(1,f);

    auto alive=genAliveBroadcast(1,"a","123",2);
    memberlistBroadcast m1("a",alive,-1);

    auto dead=genDeadBroadcast(2,"a","b");
    memberlistBroadcast m2("b",dead,-1);

    bq.QueueBroadcast(m1);
    bq.QueueBroadcast(m2);

    //bq.Prune(1);

    ComBroadcast cbc;

    bq.GetBroadcasts(0,1000,cbc);

    cout<<cbc.DebugString()<<endl;
}