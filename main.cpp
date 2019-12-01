// Patrick Sheehan

#include "huff.h"

#include <algorithm>
#include <bitset>
#include <cstring>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unistd.h>
#include <cstdlib>
#include <queue>
#include <cmath>
#include <sys/time.h>
#include <omp.h>
#include <utility>
#include <vector>


void putOut();
Node *constructHeap();

unsigned int frequencies[256] = {0};
std::string codebook[256];
std::pair<int, int> newcodebook[256];
std::vector<char> bitvec;

typedef enum { ENCODE, DECODE } MODES;

std::ifstream input_file;
std::ofstream output_file;

void compress() {
  uint8_t nextChar;
  input_file >> std::noskipws;
  while (input_file >> nextChar)
    frequencies[nextChar]++;

  Node *root = constructHeap();
  std::string code;
  root->fillCodebook(newcodebook, code, bitvec);

  putOut();
}

void putOut() {
  output_file << "HUFFMA3" << '\0';

  unsigned int i;
  for (i = 0; i < 256; i++) {
    output_file << (char)(0x000000ff & frequencies[i]);
    output_file << (char)((0x0000ff00 & frequencies[i]) >> 8);
    output_file << (char)((0x00ff0000 & frequencies[i]) >> 16);
    output_file << (char)((0xff000000 & frequencies[i]) >> 24);
  }

  unsigned char nextChar;
  char nextByte = 0;
  int bitCounter = 0;

  input_file.clear();
  input_file.seekg(0);
  input_file >> std::noskipws;
  while (input_file >> nextChar) {
    int beg = newcodebook[nextChar].first;
    int len = newcodebook[nextChar].second;
    int rst = newcodebook[nextChar].second;
#define rd (len - rst)
#define lsr(x, n) (char)(((unsigned char)x) >> (n))
    int bit_provide = std::min(8, len);
    while (rst) {
      int bit_needed = 8 - bitCounter;
      nextByte |=
          (lsr(bitvec[beg + (rd / 8)], (8 - bit_provide)) << bitCounter);
      rst -= std::min(bit_provide, bit_needed);
      if (bit_provide >= bit_needed) {
        bit_provide -= bit_needed;
        if (!bit_provide)
          bit_provide = std::min(8, rst);
        output_file << nextByte;
        nextByte = bitCounter = 0;
      } else {
        bitCounter += bit_provide;
        bit_provide = std::min(8, rst);
      }
    }
  }
  if (bitCounter)
    output_file << nextByte;
}

void decompress() {
  input_file >> std::noskipws;
  char magic[8];
  input_file.read(magic, 8);
  char nextByte;
  for (int i = 0; i < 256; i++) {
    input_file.read((char *)&frequencies[i], 4);
  }

  Node *root = constructHeap();
  std::string code;
  root->fillCodebook(codebook, code);
  std::unordered_map<std::string, int> codebook_map;

  for (int i = 0; i < 256; ++i) {
    if (codebook[i] != "") {
      codebook_map[codebook[i]] = i;
    }
  }

  while (input_file >> nextByte) {
    for (int i = 0; i < 8; i++) {
      if ((nextByte >> i) & 0x01)
        code += '1';
      else
        code += '0';

      auto exist = codebook_map.find(code);
      if (exist != codebook_map.end()) {
        int index = exist->second;
        if (frequencies[index]--) {
          output_file << (unsigned char)index;
          code.clear();
        } else {
          return;
        }
      }
    }
  }
}

Node *constructHeap() {
  struct timeval start, end;
  auto cmp = [](Node *a, Node *b) { return *a > *b; };
  std::priority_queue<Node *, std::vector<Node *>, decltype(cmp)> minHeap(cmp);
  Node *nextNode;
  for (int i = 0; i < 256; i++) {
    if (frequencies[i]) {
      nextNode = new Node(i, frequencies[i]);
      minHeap.push(nextNode);
    }
  }

  Node *node1;
  Node *node2;
  Node *merged;
  while (minHeap.size() > 1) {
    node1 = minHeap.top();
    minHeap.pop();
    node2 = minHeap.top();
    minHeap.pop();
    merged = new Node(node1, node2);
    minHeap.push(merged);
  }

  return minHeap.top();
}

void usage() {
  std::cout << "Usage:\n"
               "  -c [be compressed file] : Compress file to output file.\n"
               "  -d [decompressed file]  : Decompress file to output file.\n"
               "  -o [output file]        : Specify the output file name.\n"
               "  -h                      : Command line options.\n\n"
               "e.g.\n"
               " ./huffman -c a.txt - o a.compres.txt\n"
               "or \n"
               " ./huffman - d a.compress.txt -o a.txt\n";
}

int main(int argc, char *argv[]) {
  MODES mode = ENCODE;
  char *input_string = nullptr, *output_string = nullptr;

  int opt;
  while ((opt = getopt(argc, argv, "c:d:o:h")) != -1) {
    switch (opt) {
    case 'c':
      mode = ENCODE;
      goto FILE;
    case 'd':
      mode = DECODE;
    FILE:
      if (input_string != nullptr) {
        fprintf(stderr, "Multiple input files not allowed");
        exit(-1);
      }

      input_string = (char *)malloc(strlen(optarg) + 1);
      strcpy(input_string, optarg);
      break;

    case 'o':
      if (output_string != nullptr) {
        fprintf(stderr, "Multiple ouput files not allowed");
        exit(-1);
      }

      output_string = (char *)malloc(strlen(optarg) + 1);
      strcpy(output_string, optarg);
      break;

    case 'h':
      usage();
      exit(0);
    }
  }

  if (input_string == nullptr) {
    fprintf(stderr, "Input file must be provided\n");
    exit(-1);
  }

  if (output_string == nullptr) {
    fprintf(stderr, "Output file must be provided\n");
    exit(-1);
  }

  // open the input and output file
  input_file.open(input_string);
  if (!input_file)
    perror("Opening input file");

  output_file.open(output_string);
  if (!output_file)
    perror("Opening output file");

  if (mode == ENCODE) {
    compress();
  } else if (mode == DECODE) {
    decompress();
  }
  return 0;
}
