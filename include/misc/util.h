#ifndef _UTIL_H
#define _UTIL_H

#include "node.h"
#include "config.h"

#include <iostream>
#include <vector>
#include <cmath>

#include <arpa/inet.h>
#include <string.h>

bool hasPort(const std::string &s);

void ensurePort(std::string &s, uint16_t port);

struct sockaddr_in resolveAddr(const std::string &s);

uint32_t retransmitLimit(uint8_t retransmitMult, uint32_t n);

std::vector<nodeState> kRandomNodes(uint8_t k, std::vector<std::shared_ptr<nodeState>> &nodes, std::function<bool(std::shared_ptr<nodeState>)> exclude);

// suspicionTimeout computes the timeout that should be used when
// a node is suspected
int64_t suspicionTimeout(uint32_t suspicionMult, uint32_t n, int64_t interval);

std::string LogCoon(int connfd);

std::string LogAddr(const sockaddr_in &remote_addr);

#endif