// Patrick Sheehan

#include "huff.h"
#include <iostream>
using namespace std;

void Node::fillCodebook(string *codebook, string &code) {
  if (!leftC && !rightC) {
    codebook[data] = code;
    return;
  }
  if (leftC) {
    code += '0';
    leftC->fillCodebook(codebook, code);
    code.erase(code.end() - 1);
  }
  if (rightC) {
    code += '1';
    rightC->fillCodebook(codebook, code);
    code.erase(code.end() - 1);
  }
}

void Node::fillCodebook(pair<int, int> *codebook, string &code,
                        vector<char> &bitvec) {
  int begin = bitvec.size();
  if (!leftC && !rightC) {
    /*encode here, store to bit vec and */
    char byte = 0, bitc = 0;
    for (size_t i = 0; i < code.size(); i++, bitc++) {
      // for(int i = code.size() - 1; i >= 0; i--, bitc++){
      if (bitc == 8)
        bitvec.push_back(byte), bitc = byte = 0;
      // if(code[code.size() - 1 - i] == '1') byte |= 0x1 << bitc;
      if (code[i] == '1')
        byte |= 0x1 << bitc;
    }
    if (bitc)
      bitvec.push_back(byte << (8 - bitc));
    codebook[data] = make_pair(begin, code.size());
    // codebook[data] = code;
    return;
  }
  if (leftC) {
    code += '0';
    leftC->fillCodebook(codebook, code, bitvec);
    code.erase(code.end() - 1);
  }
  if (rightC) {
    code += '1';
    rightC->fillCodebook(codebook, code, bitvec);
    code.erase(code.end() - 1);
  }
}

Node::Node(Node *rc, Node *lc) {
  frequency = rc->frequency + lc->frequency;
  rightC = rc;
  leftC = lc;
  min = (rc->min < lc->min) ? rc->min : lc->min;
}

void Heap::push(Node *newNode) {
  int currentHeapNode = ++heapSize;
  while (currentHeapNode != 1 && *minHeap[currentHeapNode / 2] > *newNode) {
    minHeap[currentHeapNode] = minHeap[currentHeapNode / 2];
    currentHeapNode = currentHeapNode / 2;
  }
  minHeap[currentHeapNode] = newNode;
}

void Heap::pop() {
  Node *lastNode = minHeap[heapSize];
  minHeap[heapSize--] = minHeap[1];
  int currentHeapNode = 1;
  int child = 2;

  while (child <= heapSize) {
    if (child<heapSize && * minHeap[child]> * minHeap[child + 1])
      child++;

    if (*minHeap[child] > *lastNode)
      break;

    minHeap[currentHeapNode] = minHeap[child];
    currentHeapNode = child;
    child *= 2;
  } // while not at end of heap

  minHeap[currentHeapNode] = lastNode;
}

bool Node::operator>(const Node &rhs) {
  if (frequency > rhs.frequency)
    return true;
  if (frequency < rhs.frequency)
    return false;
  if (frequency == rhs.frequency)
    if (min > rhs.min)
      return true;
  return false;
}
