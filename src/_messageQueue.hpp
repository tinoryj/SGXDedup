#ifndef _MESSAGEQUEUE_HPP
#define _MESSAGEQUEUE_HPP

#include<cstdio>
#include<cstring>
#include<algorithm>
#include<iostream>
#include<string>
#include<vector>
#include<stack>
#include<bitset>
#include<cstdlib>
#include<cmath>
#include<set>
#include<list>
#include<deque>
#include<map>
#include<queue>

#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>

#include "configure.hpp"
#include "chunk.hpp"

template<class T>
class MessageQueue {
private:
	boost::lockfree::queue<T> mq;
public:
	void push(T data){
		mq.push(data);
	}
	T pop(){
		T ans;
		mq.pop(ans);
		return ans;
	}
};

#endif