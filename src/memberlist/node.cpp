#include <memberlist/node.h>
#include <memberlist/memberlist.h>
#include <type/genmsg.h>

#include <format>


// pushPullNode does a complete state exchange with a specific node.
void memberlist::pushPullNode(struct sockaddr_in& remote_node,bool join){
    auto pushpull=sendAndReceiveState(remote_node,join);

    mergeRemoteState(pushpull);
}

void memberlist::aliveNode(Alive &a,int notifyfd,bool bootstrap){
    lock_guard<mutex> l(nodeMutex);
    NodeState *state;
    bool updateNode=false;

	// It is possible that during a Leave(), there is already an aliveMsg
	// in-queue to be processed but blocked by the locks above. If we let
	// that aliveMsg process, it'll cause us to re-join the cluster. This
	// ensures that we don't.
    if(leave.load()&&a.node()==config.Name){
        return;
    }

    // Check if we've never seen this node before, and if not, then
	// store this node in our node map.
    if(nodeMap.find(a.node())==nodeMap.end()){
        state=new NodeState(Node(a.node(),a.addr(),a.port()),a.incarnation(),StateDead);

        nodeMap[a.node()]=state;
        
		// Add this state to random offset. This is important to ensure
		// the failure detection bound is low on average. If all
		// nodes did an append, failure detection bound would be
		// very high.
        size_t n=sizeof(nodes);
        size_t offset=(size_t) rand()/RAND_MAX *n;
        nodes[n]=nodes[offset];
        nodes[offset]=state;

        numNodes.fetch_add(1);
    }
    else{
        state=nodeMap[a.node()];

        // Check if this address is different than the existing node unless the old node is dead
        if(a.addr()!=state->N.Addr||a.port()!=state->N.Port){
            // Fixme: this has not been implemented
            // If DeadNodeReclaimTime is configured, check if enough time has elapsed since the node died.
            bool canReclaim=true;

            // Allow the address to be updated if a dead node is being replaced.
            if(state->State==StateDead||state->State==StateLeft&&canReclaim){
                updateNode=true;
                logger<<format("[INFO] memberlist: Updating address for left or failed node {} from {}:{} to {}:{}",a.node(),state->N.Addr,state->N.Port,a.addr(),a.port())<<endl;
            }else{
                logger<<format("[ERR] memberlist: Conflicting address for {} Mine: {}:{} Theirs: {}:{} Old state: {}",a.node(),state->N.Addr,state->N.Port,a.addr(),a.port(),state->State)<<endl;
            }
        }
    }

    bool isLocalNode= state->N.Name==config.Name;
    // Bail if strictly less and this is about us
    if(isLocalNode){
        if(a.incarnation()<state->Incarnation){
            return;
        }
    }
    // Bail if the incarnation number is older, and this is not about us
    else{
        if(a.incarnation()<=state->Incarnation&&!updateNode){
            return;
        }
    }

    //1. isLocalNode && a.incarnation()>=state->Incarnation
    //2. !isLocalNode && a.incarnation()>state->Incarnation||updateNode

    // Clear out any suspicion timer that may be in effect.
    nodeTimers.erase(a.node());

    // If this alive msg is received from other node, then bootstrap is false;
    // If this alive msg is generated by ourself to broadcast, then bootstrap is true.
    if(!bootstrap&&isLocalNode){

        // If the Incarnation is the same, we need special handling, since it
		// possible for the following situation to happen:
		// 1) Start with configuration C, join cluster
		// 2) Hard fail / Kill / Shutdown
		// 3) Restart with configuration C', join cluster
		//
		// In this case, other nodes and the local node see the same incarnation,
		// but the values may not be the same. For this reason, we always
		// need to do an equality check for this Incarnation. In most cases,
		// we just ignore, but we may need to refute.
		
        if(a.incarnation()==state->Incarnation){
            return;
        }
        refute(state,a.incarnation());
        logger<<format("[WARN] memberlist: Refuting an alive message for '{}' ({}:{})",a.node(),a.addr(),a.port())<<endl;
    }else{
        auto compoundMsg=genCompound();
        auto aliveMsg=genAlive(a);
        addMessage(compoundMsg,aliveMsg);
        encodeBroadcastNotify(a.node(),compoundMsg,notifyfd);

        // Update the state and incarnation number
        state->Incarnation=a.incarnation();
        state->N.Addr=a.addr();
        state->N.Port=a.port();
        if(state->State!=StateAlive){
            state->State=StateAlive;
            state->StateChange=chrono::duration_cast<int64_t,chrono::microseconds>(chrono::system_clock::now())
        }
    }

}

// refute gossips an alive message in response to incoming information that we
// are suspect or dead. It will make sure the incarnation number beats the given
// accusedInc value, or you can supply 0 to just get the next incarnation number.
// This alters the node state that's passed in so this MUST be called while the
// nodeLock is held.
void memberlist::refute(NodeState *me, uint32_t accusedInc){
    uint32_t inc=incarnation.load();
    if(accusedInc>=inc){
        inc=skipIncarnation(accusedInc-inc+1);
    }
    me->Incarnation=inc;

    auto compoundMsg=genCompound();
    auto aliveMsg=genAlive(inc,me->N.Name,me->N.Addr,me->N.Port);

    addMessage(compoundMsg,aliveMsg);
    encodeAndBroadcast(me->N.Name,compoundMsg);
}