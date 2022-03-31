#include "type/msgtype.pb.h"
#include <iostream>
using namespace std;

#define SWITCH(cond)\
    switch (md.head()){\
        case MessageData_MessageType::MessageData_MessageType_pingMsg:\
            \
            break;\
        case MessageData_MessageType::MessageData_MessageType_indirectPingMsg:\
                \
            break;\
        case MessageData_MessageType::MessageData_MessageType_ackRespMsg:\
                \
            break;\
        case MessageData_MessageType::MessageData_MessageType_suspectMsg:\
            \
        case MessageData_MessageType::MessageData_MessageType_aliveMsg:\
\
        case MessageData_MessageType::MessageData_MessageType_deadMsg:\
\
        case MessageData_MessageType::MessageData_MessageType_pushPullMsg:\
\
        case MessageData_MessageType::MessageData_MessageType_userMsg:\
\
        case MessageData_MessageType::MessageData_MessageType_nackRespMsg:\
\
        case MessageData_MessageType::MessageData_MessageType_errMsg:\
\
        default:\
    }\

void onReceive(const string &s){
    MessageData md;
    if(md.ParseFromString(s)==false){
        cout<<"ParseFromString Error!"<<endl;
        return;
    }
    SWITCH(md.head());
}

void onReceive(int fd){
    MessageData md;
    if(md.ParseFromFileDescriptor(fd)==false){
        cout<<"ParseFromFileDescriptor Error!"<<endl;
        return;
    }
    SWITCH(md.head());
}

void beforeSend(const MessageData &md,string *s){
    if(md.SerializeToString(s)==false){
        cout<<"SerializeToString Error!"<<endl;
        return;
    }
}

string beforeSend(const MessageData &md){
    string s=md.SerializeAsString();
    if (s.empty()){
        cout<<"SerializeAsString Error!"<<endl;
        return;
    }

}