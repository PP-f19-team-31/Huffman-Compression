// Patrick Sheehan

#include "huff.h"
#include <iostream>
#include <map>

using namespace std;

void Node::fillCodebook(string *codebook, string &code) {
  if (!left && !right) {
    codebook[data] = code;
    return;
  }
  if (left) {
    code += '0';
    left->fillCodebook(codebook, code);
    code.erase(code.end() - 1);
  }
  if (right) {
    code += '1';
    right->fillCodebook(codebook, code);
    code.erase(code.end() - 1);
  }
}

void Node::fillCodebook(pair<int, int> *codebook, string &code,
                        vector<char> &bitvec) {
  int begin = bitvec.size();
  if (!left && !right) {
    /*encode here, store to bit vec and */
    char byte = 0, bitc = 0;
    for (size_t i = 0; i < code.size(); i++, bitc++) {
      if (bitc == 8)
        bitvec.push_back(byte), bitc = byte = 0;
      if (code[i] == '1')
        byte |= 0x1 << bitc;
    }
    if (bitc)
      bitvec.push_back(byte << (8 - bitc));
    codebook[data] = make_pair(begin, code.size());
    return;
  }
  if (left) {
    code += '0';
    left->fillCodebook(codebook, code, bitvec);
    code.erase(code.end() - 1);
  }
  if (right) {
    code += '1';
    right->fillCodebook(codebook, code, bitvec);
    code.erase(code.end() - 1);
  }
}

int Node::fillCodebook(pair<int, int> *codebook, char *bitvec, int cur,
                       int len) {
  int pre = cur;
  static map<int, int> idx2len;
  if (!left && !right) {
    codebook[data] = make_pair(cur, len);
    idx2len[cur] = len;
    if (len % 8)
      bitvec[cur + len / 8] <<= (8 - len % 8);
    return cur + (len / 8) + ((len % 8) ? 1 : 0);
  }

  if (left)
    cur = left->fillCodebook(codebook, bitvec, cur, len + 1);

  if (right) {
    if (pre != cur) {
      int count = len, ptr = 0;
      while (count >= 8)
        bitvec[cur + ptr] = bitvec[pre + ptr], ptr++, count -= 8;
      if (ptr == idx2len[pre] / 8)
        for (int i = 0, offset = idx2len[pre] - ptr * 8; i < count; i++)
          bitvec[cur + len / 8] |=
              (bitvec[pre + ptr] & (0x1 << (8 - offset + i))) >> (8 - offset);
      else
        for (int i = 0; i < count; i++)
          bitvec[cur + len / 8] |= bitvec[pre + ptr] & (0x1 << i);
    }
    bitvec[cur + len / 8] |= 0x1 << (len % 8);
    cur = right->fillCodebook(codebook, bitvec, cur, len + 1);
  }
  return cur;
}

Node::Node(Node *lc, Node *rc) : left(lc), right(rc) {
  freq = lc->freq + rc->freq;
  left = lc, right = rc;
  data = min(lc->data, rc->data);
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
  return freq == rhs.freq ? data > rhs.data : freq > rhs.freq;
}
