#include <misc/suspicion.h>
using namespace std;

suspicion::suspicion(const string& from_, uint8_t k_,int64_t min_,int64_t max_,function<void(suspicion *)> f_)
:n(0),k(k_),min(min_),max(max_),f(bind(f_,this)),t(k_<1?min_:max_,f,nullptr,false)
{
    confirmations.insert(from_);
    start=chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count();
    t.Run();
}

int64_t suspicion::remainingSuspicionTime(){
    auto frac=log(n.load()+1)/log(k+1);
    int64_t timeout=max-frac*(max-min);

    int64_t now=chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count();
    int64_t elapsed=now-start;

    int64_t remain=timeout-elapsed;
    return remain;
}

uint32_t suspicion::Load(){
    return n.load();
}

bool suspicion::Confirm(const string& from_){
    // If we've got enough confirmations then stop accepting them.
    if(n.load()>k){
        return false;
    }

    // Only allow one confirmation from each possible peer.
    if(confirmations.find(from_)!=confirmations.end()){
        return false;
    }

    confirmations.insert(from_);
    n.fetch_add(1);

    int64_t remain=remainingSuspicionTime();
    if(t.Stop()){
        if(remain>0){
            t.Restart(remain);
        }else{
            f();
        }
    }
    
    return true;
}