#include <misc/suspicion.h>
#include <map>
#include <atomic>

using namespace std;

int main(){
    map<string,shared_ptr<suspicion>> nodeTimers;
    auto f =[] (suspicion *s){

    };

    nodeTimers["9999"]=make_shared<suspicion>("9999", 1, 24000000, 144000000, f);

    return 0;
}