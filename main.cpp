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
#include <algorithm>
#define num_of_thread 16
#define BUFFER_SIZE 20000
char buffer[BUFFER_SIZE];
void putOut();
Node *constructHeap();

unsigned int frequencies[256] = {0};
std::string codebook[256];
std::pair<int, int> newcodebook[256];
std::vector<char> bitvec;

typedef enum { ENCODE, DECODE } MODES;

struct CompareFirst
{ 
  CompareFirst(int val) : val_(val) {}
  bool operator()(const std::pair<unsigned long long, unsigned long long>& elem) const {
    return val_ == elem.first;
  }
  private:
  unsigned long long val_;
};

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
    output_file << (char)((0x00ff0000 & frequencies[i]) >> num_of_thread);
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
  std::vector<int> indices_codewords[num_of_thread];

  //std::vector<std::pair<unsigned long long, unsigned long long>> indexs[num_of_thread];
  std::vector<unsigned long long> indexs[num_of_thread];

  do {
    input_file.read(buffer, sizeof(buffer));
#pragma omp parallel num_threads(num_of_thread) private(code)
{
    code = "";
    int tid = omp_get_thread_num();
    int tot = omp_get_num_threads();
    char nextByte;
    bool sync = false;
    int begin = input_file.gcount() / tot * tid;
    int end   = input_file.gcount() / tot * (tid+1);
#pragma omp master
{
    begin = 0;
    end = input_file.gcount();
}
    unsigned long long bit = 0;

    for ( int byte = begin; byte < end; byte++) {
      if ( sync ) {
	sync = false; // reset
      } else {
	bit = 0;
      }
      nextByte = buffer[byte];
      for (; bit < 8; bit++) {
        code += ((nextByte >> bit) & 0x01)?'1':'0';

	auto exist = codebook_map.find(code);
	if (exist != codebook_map.end()) {
	  int index = exist->second;

	  /* store decoded char */
	  indices_codewords[tid].push_back(index);
	  /* store bit_index and code length */
	  indexs[tid].push_back(byte*8 + bit - code.size() +1);
#pragma omp master
{
	//unsolved carriage return character index = 13
	output_file << (unsigned char)index;
	for ( int t1 = 1; t1 < num_of_thread; t1++ ) {
	    size_t end_ = indexs[t1].size();
	    
	  for ( size_t t2 = 0; t2 < end_; t2++ ) {
	      if ( indexs[t1][t2] == (byte*8 + bit+1) ) { // synchronization pointi
		while (t2 != end_-1) {
		    output_file << (unsigned char) indices_codewords[t1][t2];
		    t2++;
		}
		byte = indexs[t1][t2]/8;
		bit  = indexs[t1][t2]%8+1;
		sync = true;
		break;
	      }
	    }
	}
}
	  code.clear();
	}
      }
    }
} // parallel end here
     for ( int i = 0; i < num_of_thread; ++i ) {
       indices_codewords[i].erase( indices_codewords[i].begin(), indices_codewords[i].end());
       indexs[i].erase(indexs[i].begin(), indexs[i].end());
     }
  } while(input_file);
}

Node *constructHeap() {
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

  output_file.open(output_string, std::ios::binary);
  if (!output_file)
    perror("Opening output file");

  if (mode == ENCODE) {
    compress();
  } else if (mode == DECODE) {
    decompress();
  }
  return 0;
}
