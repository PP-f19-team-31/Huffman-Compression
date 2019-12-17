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
