// This is a Test file for config.cpp
#include <memberlist/config.h>

int main(){
    auto c1=DefaultLANConfig();
    auto c2=DefaultWANConfig();
    auto c3=DefaultLocalConfig();
    return 0;
}