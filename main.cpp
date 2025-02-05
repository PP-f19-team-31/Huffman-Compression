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

#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <omp.h>
#include <queue>
#include <sys/time.h>
#include <unistd.h>
#include <unordered_map>
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

#define NUM_BLOCKS 48

void compress() {
  struct timeval start, end;
  START_TIME(start);
  input_file.seekg(0, std::ifstream::end);
  size_t size_of_file = input_file.tellg();
  input_file.seekg(0);
  input_file >> std::noskipws;

  // split the file into blocks based on number of threads
  size_t avg_chunk_size = size_of_file / NUM_BLOCKS;
  size_t *chunk_sizes = (size_t *)malloc(NUM_BLOCKS * sizeof *chunk_sizes);
  size_t *from = (size_t *)malloc(NUM_BLOCKS * sizeof *from);
  size_t *to = (size_t *)malloc(NUM_BLOCKS * sizeof *to);

  // compute the corresponding chunk_size for each thread
  from[0] = 0;
  for (size_t i = 0; i < NUM_BLOCKS; ++i) {
    chunk_sizes[i] =
        i < size_of_file % NUM_BLOCKS ? avg_chunk_size + 1 : avg_chunk_size;
    to[i] = from[i] + chunk_sizes[i] - 1;
    from[i + 1] = to[i] + 1;
  }

  // reading from file into buffer
  unsigned char *buffer = new unsigned char[size_of_file];
  input_file.read((char *)buffer, size_of_file);
  END_TIME("reading file into buffer", end);

  START_TIME(start);
  // frequenices counting (parallel)
  unsigned int frequencies_threads[NUM_BLOCKS][256] = {0};
#pragma omp parallel for num_threads(16)
  for (size_t i = 0; i < NUM_BLOCKS; ++i) {
    for (size_t j = from[i]; j < to[i] + 1; ++j) {
      frequencies_threads[i][buffer[j]]++;
    }
  }

  // adding up frequencies array
  for (size_t i = 0; i < NUM_BLOCKS; ++i) {
    for (size_t j = 0; j < 256; ++j) {
      frequencies[j] += frequencies_threads[i][j];
    }
  }
  END_TIME("counting frequencies", end);

  START_TIME(start);
  Node *root = constructHeap();
  END_TIME("construct heap", end);

  START_TIME(start);
  std::string code;
  root->fillCodebook(newcodebook, code, bitvec);
  END_TIME("fill codebook", end);

  START_TIME(start);
  putOut();
  END_TIME("flush to output", end);
}

#define BLOCK_N (32)
#define BLOCK_SIZE (50 << 10 << 10)
#define BUFFER_SIZE ((BLOCK_N) * (BLOCK_SIZE))

char buffer[BUFFER_SIZE];

typedef struct encode_block {
  bool lack;
  std::vector<char> bytes;
  encode_block() : lack(false){};
  void reset() { bytes.clear(), lack = false; }
} encode_block;

auto encode(encode_block &block, char *block_beg, char *block_end,
            int bitCounter) {
  block.reset();
  char nextByte = 0;
  for (char *chr = block_beg; chr != block_end; chr++) {
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
        block.bytes.push_back(nextByte);
        nextByte = bitCounter = 0;
      } else {
        bitCounter += bit_provide;
        bit_provide = std::min(8, rst);
      }
    }
  }
  if (bitCounter)
    block.bytes.push_back(nextByte), block.lack = true;
  return block;
}

int get_index(int len, int size, int nth) {
  int mod = len % size;
  int p = len / size;
  int floorp = p, ceilp = p + (mod ? 1 : 0);
  return ceilp * std::min(mod, nth) + floorp * std::max(0, nth - mod);
}

auto partition(int n, encode_block *blocks, char *buffer, int len,
               int &bitCounter) {
  int begs[n], ends[n], counters[n];
  for (int wrank = 0; wrank < n; wrank++) {
    int bits = 0;
    int beg = get_index(len, n, wrank);
    int end = get_index(len, n, wrank + 1);
    for (int i = beg; i != end; i++)
      bits += newcodebook[(unsigned char)buffer[i]].second;
    begs[wrank] = beg;
    ends[wrank] = end;
    counters[wrank] = bitCounter;
    bitCounter = (bitCounter + bits) % 8;
  }

#pragma omp parallel for num_threads(8)
  for (int i = 0; i < n; i++) {
    encode(blocks[i], buffer + begs[i], buffer + ends[i], counters[i]);
  }

  return blocks;
}

void flush_blocks(char &prefix, encode_block *blocks, int n) {
  for (int i = 0; i < n; i++) {
    if (prefix)
      blocks[i].bytes[0] |= prefix, prefix = 0;
    if (blocks[i].lack)
      prefix = blocks[i].bytes.back(), blocks[i].bytes.pop_back();
    output_file.write(&(blocks[i].bytes[0]), blocks[i].bytes.size());
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
  encode_block blocks[BLOCK_N];
  do {
    input_file.read(buffer, sizeof(buffer));
    partition(BLOCK_N, blocks, buffer, input_file.gcount(), bitCounter);
    flush_blocks(prefix, blocks, BLOCK_N);
  } while (input_file);
  if (blocks[BLOCK_N - 1].lack)
    output_file.write(&prefix, 1);
}

#define ceild(n, d) ceil(((double)(n)) / ((double)(d)))

// 2002 Journal
// Parallel Huffman Decoding with Application to JPEG Files
void decompress() {
  struct timeval start, end;
  START_TIME(start)
  // -----------------------------------------------------------------------
  // calculate the encoded code size (exclude the magic number and frequencies)
  // -----------------------------------------------------------------------
  input_file.seekg(0, std::ifstream::end);
  size_t size_of_file = input_file.tellg();
  input_file.seekg(0);
  input_file >> std::noskipws;
  char magic[8];
  input_file.read(magic, 8); // 8 bytes
  for (int i = 0; i < 256; i++) {
    input_file.read((char *)&frequencies[i], 4); // 4 bytes * 256 = 1024
  }
  size_t encoded_size = size_of_file - 1032;
  END_TIME("reading file into buffer", end);

  // 1024 + 8 = 1032 bytes
  // -----------------------------------------------------------------------

  START_TIME(start);
  Node *root = constructHeap();
  END_TIME("construct heap", end);
  {
    std::string code;
    START_TIME(start);
    root->fillCodebook(codebook, code);
    END_TIME("fill codebook", end);
  }
  std::unordered_map<std::string, int> codebook_map;

  START_TIME(start);
  for (int i = 0; i < 256; ++i) {
    if (codebook[i] != "") {
      codebook_map[codebook[i]] = i;
      // std::cout << (unsigned char)i << " : " << codebook[i] << std::endl;
    }
  }
  END_TIME("build code maping table <code, char>", end);

  START_TIME(start);
  // -----------------------------------------------------------------------
  //                 split the input_file into blocks
  // -----------------------------------------------------------------------
  // split the file into blocks based on number of threads
  // size_t block_size = 31; // (bytes) 31 256
  size_t block_size = 256; // (bytes) 31 256
  size_t num_of_block = ceild(encoded_size, block_size);

  // reading from file into buffer
  unsigned char *buffer = new unsigned char[encoded_size];
  input_file.read((char *)buffer, size_of_file);

  size_t last_block_size = encoded_size - (num_of_block - 1) * block_size;

  std::vector<std::vector<std::pair<unsigned long long, int>>>
      indices_codewords(num_of_block);

  std::vector<std::pair<unsigned long long, unsigned long long>> indexs(
      num_of_block);

  // -----------------------------------------------------------------------
  //                 starting parallel for each block_i
  // -----------------------------------------------------------------------
#pragma omp parallel for num_threads(8)
  for (size_t block_idx = 0; block_idx < num_of_block; ++block_idx) {
    unsigned long long bit_index = 0;
    char nextByte;
    std::string code;
    unsigned long long start_index = block_idx * block_size;
    unsigned long long end_index =
        start_index +
        (block_idx != num_of_block - 1 ? block_size : last_block_size);
    unsigned long long idx = start_index;
    unsigned long long last_code_len = 0;
    {
      // idx is a global index of the buffer
      for (; idx < end_index; ++idx) {
        // extract a byte from buffer
        nextByte = buffer[idx];
        // check the buffer[j] in byte (bit by bit)
        // to check weather there is a key stored in codebook_map
        for (int k = 0; k < 8; ++k) {
          code += ((nextByte >> k) & 0x01) ? '1' : '0';
          // find the code in codebook
          auto exist = codebook_map.find(code);
          if (exist != codebook_map.end()) {
            // found a codeword, extract its index
            int decoded_char = exist->second;
            // record the bit index of the codeword
            bit_index = idx * 8 + k - code.size() + 1;
            indices_codewords[block_idx].push_back(
                std::make_pair(bit_index, decoded_char));
            last_code_len = code.size();
            code.clear();
          }
        }
      }

      unsigned long long bidx =
          indices_codewords[block_idx].back().first + last_code_len;
      indexs[block_idx] = std::make_pair(bidx / 8, bidx % 8);
    }
  }
  END_TIME("parallel interpret each blocks", end);

  START_TIME(start);
  unsigned long long last_bit =
      indices_codewords[num_of_block - 1].back().first;
  bool shit = false;
  size_t block_idx = 0;
  size_t it_offset = 0;

  // flush the codewords
  auto ps = indices_codewords[block_idx];
  auto p = std::next(ps.begin(), it_offset);
  std::for_each(p, ps.end(), [](std::pair<unsigned long long, int> key_pair) {
    output_file << (unsigned char)(key_pair.second);
  });

  std::string code;
  unsigned long long last_idx = 0;
DOTAIL:
  // rollback
  auto idx = indexs[block_idx].first;
  auto offset = indexs[block_idx].second;
  code.clear();
  auto bit_index = indices_codewords[block_idx].back().first;

  // increase the block index
  block_idx++;
  if (bit_index % block_size != 0) {
    unsigned long long start_index = block_idx * block_size;
    unsigned long long end_index =
        start_index +
        (block_idx != num_of_block - 1 ? block_size : last_block_size);

    // decode next codeword
    while (true) {
      for (int k = offset; k < 8; ++k) {
        // reached the last bit, stops the decompress process
        if (idx * 8 + k >= last_bit)
          goto END;

        last_idx = idx * 8 + k;
        code += ((buffer[idx] >> k) & 0x01) ? '1' : '0';
        offset++;

        // find the code in codebook
        auto exist = codebook_map.find(code);
        if (exist != codebook_map.end()) {
          // found a codeword, extract the codeword
          int decoded_cahr = exist->second;

          // flush the codeword
          output_file << (unsigned char)(decoded_cahr);
          code.clear();

        NEXTBLOCK:
          auto bit_index = idx * 8 + offset;
          int r = 0;
          for (; r < indices_codewords[block_idx].size(); r++) {
            if (bit_index <= indices_codewords[block_idx][r].first)
              break;
          }

          if (bit_index > indices_codewords[block_idx][r].first) {
            block_idx++;
            goto NEXTBLOCK;
            // fully pass the block
          }

          if (indices_codewords[block_idx][r].first == bit_index) {
            it_offset = r;

            while (true) {
              auto ps = indices_codewords[block_idx];
              auto p = ps.begin();
              std::advance(p, it_offset);
              std::for_each(p, ps.end(),
                            [](std::pair<unsigned long long, int> key_pair) {
                              output_file << (unsigned char)(key_pair.second);
                            });
              if (indices_codewords[block_idx].back().first % block_size != 0) {
                goto DOTAIL;
              } else {
                // clear
                block_idx++;
              }
            }
          }
        }
      }

      offset = 0;
      idx++;
    }
  } else {
    // match the boundary
    // but do nothing
  }
 END:
  END_TIME("sequential interpret the error blocks", end)
  return;
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
