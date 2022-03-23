#ifndef _CONFIG_H
#define _CONFIG_H
#include <iostream>
using namespace std;

typedef struct config
{
    // The name of this node. This must be unique in the cluster.
    string Name;

    // Configuration related to what address to bind to and ports to
	// listen on. The port is used for both UDP and TCP gossip. It is
	// assumed other nodes are running on this port, but they do not need
	// to.
	string BindAddr;
	int BindPort;


    /* data */
};


#endif