/*
 * Copyright (c) PP-f19-team-31, Patrick Sheehan,
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer. Redistributions in
 * binary form must reproduce the above copyright notice, this list of
 * conditions and the following disclaimer in the documentation and/or other
 * materials provided with the distribution. Neither the name of the copyright
 * holder nor the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
