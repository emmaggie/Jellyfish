#ifndef __CONCURRENT_QUEUES_HPP__
#define __CONCURRENT_QUEUES_HPP__

#include <sys/time.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
using namespace std;
#include <new>
#include <stdio.h>

/* Concurrent queue. No check whether enqueue would overflow the queue.
 */

template<class Val>
class concurrent_queue {
  Val                   **queue;
  unsigned int          size;
  unsigned int          volatile head;
  unsigned int          volatile tail;
  bool                  volatile closed;
  
public:
  concurrent_queue(unsigned int _size) : size(_size + 1), head(0), tail(0) { 
    queue = new Val *[size];
    memset(queue, 0, sizeof(Val *) * size);
    closed = false;
  }
  ~concurrent_queue() { delete [] queue; }

  void enqueue(Val *v);
  Val *dequeue();
  bool is_closed() { return closed; }
  void close() { closed = true; }
  bool has_space() { return head != tail; }
  bool is_low() { 
    int len = head - tail;
    if(len < 0)
      len += size;
    return (unsigned int)(4*len) <= size;
  }
};

template<class Val>
void concurrent_queue<Val>::enqueue(Val *v) {
  int done = 0;

  while(!done) {
    unsigned int chead = head;
    unsigned int nhead = (chead + 1) % size;

    done = __sync_bool_compare_and_swap(&queue[chead], 0, v);
    __sync_bool_compare_and_swap(&head, chead, nhead);
  }
}

template<class Val>
Val *concurrent_queue<Val>::dequeue() {
  int done = 0;
  Val *res;
  unsigned int chead, ctail, ntail;

  while(!done) {
    ctail = tail;
    chead = head;

    if(ctail != chead) {
      ntail = (ctail + 1) % size;
      done = __sync_bool_compare_and_swap(&tail, ctail, ntail);
    } else {
      return NULL;
    }
  }
  res = queue[ctail];
  queue[ctail] = NULL;
  return res;
}
  
#endif // __CONCURRENT_QUEUES_HPP__
