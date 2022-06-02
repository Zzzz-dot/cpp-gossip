#include <iostream>
#include <error.h>
#include <stdio.h>

using namespace std;
int main()
{
    errno=1;
    perror("asfas");
    cerr<<"asfsas"<<endl;
    while (true)
    {
        /* code */
    }
    return 0;
    
}