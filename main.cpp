// Patrick Sheehan

#include "huff.h"

#include <algorithm>
#include <bitset>
#include <cmath>
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
  input_file.seekg(0, std::ifstream::end);
  size_t size_of_file = input_file.tellg();
  std::cout << "size of file (bytes): " << size_of_file << "\n";
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

  Node *root = constructHeap();
  std::string code;
  root->fillCodebook(newcodebook, code, bitvec);
  putOut();
}

#define BLOCK_N (4)
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
  for (int wrank = 0; wrank < n; wrank++) {
    int bits = 0;
    int beg = get_index(len, n, wrank);
    int end = get_index(len, n, wrank + 1);
    for (int i = beg; i != end; i++)
      bits += newcodebook[(unsigned char)buffer[i]].second;
    encode(blocks[wrank], buffer + beg, buffer + end, bitCounter);
    bitCounter = (bitCounter + bits) % 8;
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
  std::cout << "encoded size (bytes): " << encoded_size << "\n";
  // 1024 + 8 = 1032 bytes
  // -----------------------------------------------------------------------

  Node *root = constructHeap();
  {
    std::string code;
    root->fillCodebook(codebook, code);
  }
  std::unordered_map<std::string, int> codebook_map;

  for (int i = 0; i < 256; ++i) {
    if (codebook[i] != "") {
      codebook_map[codebook[i]] = i;
    }
  }
  // -----------------------------------------------------------------------
  //                 split the input_file into blocks
  // -----------------------------------------------------------------------
  // split the file into blocks based on number of threads
  size_t block_size = 31; // (bytes) 31 256
  size_t num_of_block = ceild(encoded_size, block_size);

  std::cout << "block size: " << block_size << std::endl;
  std::cout << "number of block: " << num_of_block << std::endl;

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
#pragma omp parallel for num_threads(16)
  for (size_t block_idx = 0; block_idx < num_of_block; ++block_idx) {
    unsigned long long bit_index = 0;
    unsigned long long offset = 0;
    char nextByte;
    std::string code;
    unsigned long long start_index = block_idx * block_size;
    unsigned long long end_index =
        start_index +
        (block_idx != num_of_block - 1 ? block_size : last_block_size);
    unsigned long long idx = start_index;
    {
      // idx is a global index of the buffer
      unsigned long long store_idx = 0;
      for (; idx < end_index; ++idx) {
        // extract a byte from buffer
        nextByte = buffer[idx];
        // check the buffer[j] in byte (bit by bit)
        // to check weather there is a key stored in codebook_map
        offset = 0;
        int temp_offset = 0;
        for (int k = 0; k < 8; ++k) {
          code += ((nextByte >> k) & 0x01) ? '1' : '0';
          temp_offset++;
          // find the code in codebook
          auto exist = codebook_map.find(code);
          if (exist != codebook_map.end()) {
            // found a codeword, extract its index
            int decoded_char = exist->second;
            // record the bit index of the codeword
            bit_index = idx * 8 + offset;
            indices_codewords[block_idx].push_back(
                std::make_pair(bit_index, decoded_char));

            offset = temp_offset;
            store_idx = idx;

            code.clear();
          }
        }
      }
      indexs[block_idx] = std::make_pair(store_idx, offset);
    }
  }

  size_t block_idx = 0;
  size_t it_offset = 0;

  while (block_idx != num_of_block - 1) {
    // flush the codewords
    auto ps = indices_codewords[block_idx];
    auto p = ps.begin();
    std::advance(p, it_offset);
    std::for_each(p, ps.end(), [](std::pair<unsigned long long, int> key_pair) {
      output_file << (unsigned char)(key_pair.second);
    });

    // rollback
    auto idx = indexs[block_idx].first;
    auto offset = indexs[block_idx].second;
    auto bit_index = indices_codewords[block_idx].back().first;
    int nextByte = buffer[idx];
    std::string code;
    code.clear();

    // temp for chcecking
    unsigned long long start_index = block_idx * block_size;
    unsigned long long end_index =
        start_index +
        (block_idx != num_of_block - 1 ? block_size : last_block_size);

    block_idx++;
    if (bit_index != end_index * 8) {
      unsigned long long start_index = block_idx * block_size;
      unsigned long long end_index =
          start_index +
          (block_idx != num_of_block - 1 ? block_size : last_block_size);

      for (; idx < end_index; idx++) {
        // unsigned long long offset  = 0;
        // check the buffer[j] in byte (bit by bit)
        // to check weather there is a key stored in codebook_map

        for (int k = offset; k < 8; ++k) {
          code += ((buffer[idx] >> k) & 0x01) ? '1' : '0';

          offset++;
          // find the code in codebook
          auto exist = codebook_map.find(code);
          if (exist != codebook_map.end()) {
            // found a codeword, extract its index
            int decoded_char = exist->second;

            // record the bit index of the codeword
            output_file << (unsigned char)(decoded_char);
            code.clear();

            // checking weather it match or not

            auto bit_index = idx * 8 + offset;
            int r = 0;
            while (bit_index > indices_codewords[block_idx][r].first)
              r++;
            if (indices_codewords[block_idx][r].first == bit_index) {
              std::cout << "match the sync point at " << bit_index << "\n";

              it_offset = r;
              goto STOP;
            }
          }
        }
        offset = 0;
      }
    } else {
      std::cout << "match the boundary\n";

      offset = 0;
      unsigned long long start_index = block_idx * block_size;
      unsigned long long end_index =
          start_index +
          (block_idx != num_of_block - 1 ? block_size : last_block_size);
      idx = start_index;
    }
  STOP:
    std::cout << "do sth\n";
    // block_idx++;
  }
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
