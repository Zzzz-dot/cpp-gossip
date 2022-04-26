#ifndef _UTIL_H
#define _UTIL_H

#include <iostream>
#include <arpa/inet.h>
using namespace std;

struct sockaddr_in resolveAddr(const string &s){
    //cluster_add format: [name/]<ip>[:port]
    int index=s.find("/");
    string nodeName="";
    string host;
    //if cluster_addr contains nodename
    //but nodename is not used in this implemention
    if(index<s.size()){
        nodeName=s.substr(0,index);
        host=s.substr(index);
    }else{
        host=s;
    }

    ensurePort(host,DEFAULT_PORT);
    index=host.find(":");

    struct sockaddr_in cluster_addr;
    bzero((void *)&cluster_addr,sizeof(sockaddr_in));
    cluster_addr.sin_family=AF_INET;
    cluster_addr.sin_port = htons(stoul(host.substr(index)));
    if (int e = inet_pton(AF_INET,host.substr(0,index).c_str(), &cluster_addr.sin_addr) <= 0)
    {
        errno = e;
    }
    return cluster_addr;
}

void ensurePort(string &s,uint16_t port){
    if (hasPort(s)){
        return;
    }else{
        s+=":"+to_string(port);
    }
}

bool hasPort(const string &s){
    size_t index=s.find(":");
    if(index==s.size()){
        return false;
    }
    return true;
}
#endif