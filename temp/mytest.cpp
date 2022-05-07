#include <iostream>
#include <set>

using namespace std;

set<int> a={1,2,3};

int main(){
    a.insert(3);
    cout<<a.size()<<endl;
    return 0;
}