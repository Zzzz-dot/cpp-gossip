#include <iostream>
#include <memory>

using namespace std;

class A{
    public:
        A(int x){
            cout<<x<<endl;
            cout<<"A"<<endl;
        }

        ~A(){
            cout<<"~A"<<endl;
        }
};

int main()
{
    auto a=make_shared<A>(1);

    a=make_shared<A>(2);
}