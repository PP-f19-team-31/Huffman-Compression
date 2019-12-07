// Patrick Sheehan

#include <cstdlib>
#include <string>

#include <sys/time.h>
#include <vector>
#include <utility>

#define START_TIME(start)                                                      \
  { gettimeofday(&start, NULL); }

#define END_TIME(name, end)                                                    \
  {                                                                            \
    gettimeofday(&end, NULL);                                                  \
    double delta = ((end.tv_sec - start.tv_sec) * 1000000u + end.tv_usec -     \
                    start.tv_usec) /                                           \
                   1.e6;                                                       \
                                                                               \
    printf("\033[92m[%s]\033[0m Execute time: %lf\n", name, delta);            \
  }

class Node{
public:
    unsigned char data;
    unsigned freq;
    Node *left, *right;
    Node() = delete;
    Node(unsigned char d, unsigned f):
      data(d), freq(f), left(nullptr), right(nullptr){}
    Node(Node *, Node *);
    void fillCodebook(std::string *, std::string &);
    void fillCodebook(std::pair<int, int> *codebook, std::string &code, std::vector<char>& bitvec);
    void fillCodebook(std::pair<int, int> *codebook, std::string &code, char *bitvec);
    int  fillCodebook(std::pair<int, int> *codebook, char *bitvec, int beg, int len);
    bool operator> (const Node &);
};

class Heap{
    Node **minHeap;
    int heapSize;
public:
    Heap(){heapSize = 0; minHeap = new Node*[257];} // max of 255 characters
    void push(Node *);
    int size(){return heapSize;}
    void pop();
    Node * top(){return minHeap[1];}
};
