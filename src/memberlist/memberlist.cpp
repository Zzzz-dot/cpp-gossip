#include <memberlist/memberlist.h>
using namespace std;

void memberlist::newmemberlist(){
    clearmemberlist();

    // Init local address
    struct sockaddr_in local_addr;
    bzero(&local_addr, sizeof(sockaddr_in));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(config.BindPort);
    if (int e = inet_pton(AF_INET, config.BindAddr.c_str(), &local_addr.sin_addr) <= 0)
    {
        errno = e;
    }

    // Create TCP socket
    tcpfd = Socket(AF_INET, SOCK_STREAM, 0);
    Bind(tcpfd, (struct sockaddr *)&local_addr, sizeof(sockaddr));
    Listen(tcpfd, LISTENNUM);
    // Create UDP socket
    udpfd = Socket(AF_INET, SOCK_DGRAM, 0);
    Bind(udpfd, (struct sockaddr *)&local_addr, sizeof(sockaddr));
    // Create epoll
    epollfd = Epoll_create(EPOLLSIZE);

    // Add the tcpfd and udpfd to the watchlist of epollfd
    struct epoll_event ev1;
    ev1.events = EPOLLIN;
    ev1.data.fd = tcpfd;
    Epoll_ctl(epollfd, EPOLL_CTL_ADD, tcpfd, &ev1);
    struct epoll_event ev2;
    ev2.events = EPOLLIN;
    ev2.data.fd = udpfd;
    Epoll_ctl(epollfd, EPOLL_CTL_ADD, udpfd, &ev2);
};

void memberlist::clearmemberlist(){

}

// Join is used to take an existing Memberlist and attempt to join a cluster
// by contacting the given host and performing a state sync. Initially,
// the Memberlist only contains our own state, so doing this will cause
// remote node to become aware of the existence of this node, effectively
// joining the cluster.
void memberlist::join(const string& cluster_addr){
    struct sockaddr_in remote_addr=resolveAddr(cluster_addr);
}

memberlist::memberlist():scheduled(false)
{
    newmemberlist();

    schedule();
}



memberlist::~memberlist()
{
    clearmemberlist();

}

// Schedule is used to ensure the timer is performed periodically. This
// function is safe to call multiple times. If the memberlist is already
// scheduled, then it won't do anything.
void memberlist::schedule()
{
    if (scheduled)
        return;
    else
        scheduled = true;

    // probe,pushpull,gossip这几个函数可能需要转换成公有

    // Probe
    if (config.ProbeInterval > 0)
    {
        schedule_timer[0] = unique_ptr<timer>(new timer(config.ProbeInterval, bind(&memberlist::probe, this), this));
    }

    // PushPull
    if (config.PushPullInterval > 0)
    {
        schedule_timer[1] = unique_ptr<timer>(new timer(config.ProbeInterval, bind(&memberlist::pushpull, this), this, true));
    }

    // Gossip
    if (config.GossipInterval > 0)
    {
        schedule_timer[2] = unique_ptr<timer>(new timer(config.ProbeInterval, bind(&memberlist::gossip, this), this));
    }
}

// Deschedule is used to stop the background maintenance. This is safe
// to call multiple times.
void memberlist::deschedule()
{
    if (!scheduled)
        return;
    else
        scheduled = false;

    schedule_timer[0].release();
    schedule_timer[1].release();
    schedule_timer[2].release();
}

// Perform a single round of failure detection and gossip
void memberlist::probe()
{
    size_t numCheck = 0;
START:
    {
        unique_lock<mutex> l(nodeMutex);
        // Make sure we don't wrap around infinitely
        if (numCheck >= nodes.size())
        {
            l.unlock();
            return;
        }

        // Handle the wrap around case
	    if (probeIndex >= nodes.size()) {
            random_shuffle(nodes.begin(),nodes.end());
		    l.unlock();
		    probeIndex = 0;
		    numCheck++;
		    goto START;
	    }

        // Determine if we should probe this node
	    bool skip = false;
	    NodeState node; 

	    node = *nodes[probeIndex];
        l.unlock();
	    if (node.N.Name == config.Name) {
		    skip = true;
	    } else if (node.DeadOrLeft()) {
		    skip = true;
	    }

	    // Potentially skip
	    probeIndex++;
	    if (skip) {
		    numCheck++;
		    goto START;
	    }
        // Probe the specific node
	    probenode(node);
    }
}

// Send a ping request to the target node and wait for reply
void memberlist::probenode(NodeState &node)
{
    uint32_t seqno=nextSeqNum();
    auto ping=genPing(seqno,node.N.Name,config.BindAddr,config.BindPort,config.Name);

    //匿名管道
    //Empty Msg can be sent to nackfd, but not ackfd
    int ackfd[2];
    Pipe2(ackfd,O_DIRECT);
    int nackfd[2];
    Pipe(nackfd);

    //Set Probe Pipes
    setprobepipes(seqno,ackfd[1],nackfd[1],config.ProbeInterval);

    struct sockaddr_in addr=node.N.FullAddr();

    int64_t sendtime=chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count();

    int64_t deadline=sendtime+config.ProbeInterval;

    if (node.State==StateAlive){
        encodeSendUDP(udpfd,&addr,ping);
    }
    //Else, this is a suspected node
    else{
        //This two messages con be composed into one compound message, but currently our implementation doesn't support compound message yet 
        encodeSendUDP(udpfd,&addr,ping);
        auto suspect=genSuspect(node.Incarnation,node.N.Name,config.Name);
        encodeSendUDP(udpfd,&addr,suspect);
    }

    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(ackfd[0],&rset);
    struct timeval probetimeout;
    probetimeout.tv_usec=config.ProbeTimeout;

    Select(ackfd[0]+1,&rset,nullptr,nullptr,&probetimeout);
    if(FD_ISSET(ackfd[0],&rset)){
        ackMessage ackmsg;
        Read(ackfd[0],(void *)&ackmsg,sizeof(ackMessage));
        if(ackmsg.Complete==true){
            return;
        }
        //Some corner case when akcmsg.Complete==false
        else{
            Write(ackfd[1],(void *)&ackmsg,sizeof(ackMessage));
        }
    }
    //Probe Timeout
    else{
        #ifdef DEBUG
        logger<<"[DEBUG] memberlist: Failed UDP ping: "<<node.N.Name<<" (timeout reached)"<<endl;
        #endif
    }

    //IndirectProbe If True Ack is not received after ProbeTimeout
    vector<NodeState> kNodes;
    {
        lock_guard<mutex> l(nodeMutex);
        kNodes=kRandomNodes(config.IndirectChecks,[this](NodeState *n)->bool{
            return n->N.Name==this->config.Name||n->State!=StateAlive;
        });
    }

    auto indirectping=genIndirectPing(seqno,node.N.Name,node.N.Addr,node.N.Port,true,config.BindAddr,config.BindPort,config.Name);
    for(size_t i=0;i<kNodes.size();i++){
        struct sockaddr_in addr=kNodes[i].N.FullAddr();
        encodeSendUDP(udpfd,&addr,indirectping);
    }

    //TCP ping

    //Wait for IndirectPing
    ackMessage ackmsg;
    Read(ackfd[0],(void *)&ackmsg,sizeof(ackMessage));
    if(ackmsg.Complete==true){
        return;
    }

    //This node may fail, gossip suspect
    logger<<"[INFO] memberlist: Suspect "<<node.N.Name<<" has failed, no acks received!"<<endl;
    auto suspect=genSuspect(node.Incarnation,node.N.Name,config.Name);
    //suspectnode(node);
}

// setprobepipes is used to attach the ackpipe to receive a message when an ack
// with a given sequence number is received.
void memberlist::setprobepipes(uint32_t seqno,int ackpipe,int nackpipe,uint32_t probeinterval){
    //onRceive Ack Resp
    auto ackFn=[ackpipe](int64_t timestamp){
        ackMessage ackmsg(true,timestamp);
        Write(ackpipe,(void *)&ackmsg,sizeof(ackMessage));
    };

    //onReceive Nack Resp
    auto nackFn=[nackpipe](){
        Write(nackpipe,nullptr,0);
    };

    //Callback functino after prointerval, this timer is invoked only when the Ack expired
    auto f=[this,seqno,ackpipe]{
        {
            lock_guard<mutex> l(this->AckLock);
            this->AckHandlers.erase(seqno);
        }
        int64_t now=chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count();
        ackMessage ackmsg(false,now);
        Write(ackpipe,(void *)&ackmsg,sizeof(ackMessage));
    };

    //Add AckHandlers
    {
        timer t=timer(probeinterval,f,this);
        lock_guard<mutex> l(AckLock);
        AckHandlers.emplace(seqno,AckHandler(ackFn,nackFn,t));
    }
    //Start timer
    AckHandlers[seqno].t.Run();
}


vector<NodeState> memberlist::kRandomNodes(uint8_t k,function<bool(NodeState *n)> exclude){
    size_t n=nodes.size();
    vector<NodeState> kNodes;
    for (int i=0;i<3*n&&kNodes.size()<k;i++){
        int idx=rand()/RAND_MAX*n;
        if(exclude!=NULL&&exclude(nodes[idx])){
            continue;
        }

        for(int j=0;j<kNodes.size();j++){
            if(nodes[idx]->N.Name==kNodes[j].N.Name){
                continue;
            }
        }
        kNodes.push_back(*nodes[idx]);
    }
    return kNodes;
}