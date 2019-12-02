// Patrick Sheehan

#include "huff.h"

#include <assert.h>
#include <algorithm>
#include <bitset>
#include <cstring>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
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

#define BLOCK_N     (4)
#define BLOCK_SIZE  (50 << 10 << 10)
#define BUFFER_SIZE ((BLOCK_N) * (BLOCK_SIZE))

char buffer[BUFFER_SIZE];

typedef struct encode_block{
  int begbc, endbc;
  std::vector<char> bytes;
  encode_block() : begbc(0), endbc(0){};
  encode_block(int bc) : begbc(bc), endbc(0){};
} encode_block;

auto encode(char *block_beg, char *block_end, int &bitCounter){
  auto block = new encode_block(bitCounter);
  char nextByte = 0;
  for(char *chr = block_beg; chr != block_end; chr++){
    int beg = newcodebook[(unsigned char)*chr].first;
    int len = newcodebook[(unsigned char)*chr].second;
    int rst = newcodebook[(unsigned char)*chr].second;
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
        block->bytes.push_back(nextByte);
        nextByte = bitCounter = 0;
      } else {
        bitCounter += bit_provide;
        bit_provide = std::min(8, rst);
      }
    }
  }
  if (bitCounter) block->bytes.push_back(nextByte);
  block->endbc = bitCounter;
  return block;
}


int get_index(int len, int size, int nth){
  int mod = len % size;
  int p = len / size;
  int floorp = p, ceilp = p + (mod ? 1 : 0);
  return ceilp * std::min(mod, nth) + floorp * std::max(0, nth - mod);
}

auto partition(int n, char *buffer, int len, int &bitCounter){
  auto blocks = new encode_block*[n];
  for(int wrank = 0; wrank < n; wrank++){
    int bits = 0;
    int beg = get_index(len, n, wrank);
    int end = get_index(len, n, wrank + 1);
    int prebitCounter = bitCounter;
    for(int i = beg; i != end; i++)
      bits += newcodebook[(unsigned char)buffer[i]].second;
    blocks[wrank] = encode(buffer + beg, buffer + end, bitCounter);
    prebitCounter = (prebitCounter + bits) % 8;
    //printf("%d == %d? (%d, %d)\n", prebitCounter, bitCounter, beg, end);
    //assert(prebitCounter == bitCounter);
  }
  return blocks;
}

void flush_blocks(char &prefix, encode_block **blocks, int n){
  for(int i = 0; i < n; i++){
    if(blocks[i]->begbc)
      blocks[i]->bytes[0] |= prefix;
    if(blocks[i]->endbc)
      prefix = blocks[i]->bytes.back(), blocks[i]->bytes.pop_back();
    output_file.write(&(blocks[i]->bytes[0]), blocks[i]->bytes.size());
  }
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

  input_file.clear();
  input_file.seekg(0);
  input_file >> std::noskipws;

  int bitCounter = 0;
  char prefix = 0;
  encode_block **blocks;
  do {
    input_file.read(buffer, sizeof(buffer));
    blocks = partition(BLOCK_N, buffer, input_file.gcount(), bitCounter);
    flush_blocks(prefix, blocks, BLOCK_N);
  } while(input_file);
  if(blocks[BLOCK_N - 1]->endbc) output_file.write(&prefix, 1);
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
