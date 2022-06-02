#ifndef _SUSPICION_H
#define _SUSPICION_H

#include "timer.h"

#include <iostream>
#include <set>
#include <functional>
#include <cmath>
#include <atomic>
#include <chrono>

// suspicion manages the suspect timer for a node and provides an interface
// to accelerate the timeout as we get more independent confirmations that
// a node is suspect.
// Two methods for judging a suspicion is timeout:
// 1. The timer timeout;
// 2. When receive a new suspicion, update and check.
struct suspicion
{

    bool Confirm(const std::string& from_);
    uint32_t Load();
    suspicion(const std::string& from_, uint8_t k_,int64_t min_,int64_t max_,std::function<void(suspicion *)> f_);


    private:
    // n is the number of independent confirmations we've seen. This must
	// be updated using atomic instructions to prevent contention with the
	// timer callback.
    std::atomic<uint32_t> n;

    // k is the number of independent confirmations we'd like to see in
	// order to drive the timer to its minimum value.
    uint32_t k;

	// start captures the timestamp when we began the timer. This is used
	// so we can calculate durations to feed the timer during updates in
	// a way the achieves the overall time we'd like.
    int64_t start;

    // min is the minimum timer value.
    int64_t min;

    // max is the maximum timer value.
    int64_t max;

    // f is the function to call when the timer expires. We hold on to this
	// because there are cases where we call it directly.
    // IMPORTANT: f should be declared before t
    std::function<void()> f;

	// timer is the underlying timer that implements the timeout.
    onceTimer t;

	// confirmations is a map of "from" nodes that have confirmed a given
	// node is suspect. This prevents double counting.
    std::set<std::string> confirmations;
    
    // remainingSuspicionTime takes the state variables of the suspicion timer and
    // calculates the remaining time to wait before considering a node dead. The
    // return value can be negative, so be prepared to fire the timer immediately in
    // that case.
    int64_t remainingSuspicionTime();
};

#endif