#include "misc/safe_queue.h"
#include <list>
using namespace std;
int main(){
    safe_queue<int,list<int>> sq;
    auto f=[sq]{
        for (int i=0;i<1000000;i++){
            sq.push(i);
        }
    };

}