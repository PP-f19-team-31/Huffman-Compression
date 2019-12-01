// Patrick Sheehan

#include <cstdlib>
#include <string>
#include <vector>
#include <utility>

class Node{
    unsigned char data;
    unsigned int frequency;
    unsigned char min;
    Node * leftC;
    Node * rightC;
public:
    Node(){ data = frequency = min = 0, leftC = rightC = nullptr; };
    Node(const Node &n){data = n.data; frequency = n.frequency; leftC = n.leftC; rightC = n.rightC;}
    Node(unsigned char d, unsigned int f): data(d), frequency(f), min(d){ leftC = rightC = nullptr;}
    Node(Node *, Node *);
    void fillCodebook(std::string *, std::string &);
    void fillCodebook(std::pair<int, int> *codebook, std::string &code, std::vector<char>& bitvec);
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
