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
#include <mpi.h>
#include <utility>
#include <vector>
#define num_of_thread 16
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
  /* MPI */
  MPI_Init(NULL, NULL);
  int MPI_size;
  MPI_Comm_size(MPI_COMM_WORLD, &MPI_size);
  
  int MPI_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &MPI_rank);
  
  Node *root;
  size_t encoded_size, size_of_file;
  std::unordered_map<std::string, int> codebook_map;
  char magic[8];
  if ( MPI_rank == 0 ) {
    input_file.seekg(0, std::ifstream::end);
    size_of_file = input_file.tellg();
    std::cout << "size of file (bytes): " << size_of_file << "\n";
    input_file.seekg(0);
    input_file >> std::noskipws;
    input_file.read(magic, 8); // 8 bytes
    for (int i = 0; i < 256; i++) {
	    input_file.read((char *)&frequencies[i], 4); // 4 bytes * 256 = 1024
    }
  }
  /* construct heap for every rank */
  if ( MPI_rank == 0 ) {
    for (int i = 1; i < MPI_size; i++) {
      MPI_Send(&frequencies, 256, MPI_UNSIGNED, 
		      i, 0, MPI_COMM_WORLD);
    }
  } else {
    MPI_Recv(&frequencies, 256, MPI_UNSIGNED,
		    0, 0, MPI_COMM_WORLD, NULL);
  }
  root = constructHeap();
  {
	  std::string code;
	  root->fillCodebook(codebook, code);
  }
  for (int i = 0; i < 256; ++i) {
	  if (codebook[i] != "") {
		  codebook_map[codebook[i]] = i;
	  }
  }
  /* decode */
  unsigned long long block_size;
  std::string code;
  encoded_size = size_of_file - 1032;
  unsigned char *buffer;
  /* partition block */
  if ( MPI_rank == 0 ) {
    buffer = new unsigned char [encoded_size];
    input_file.read((char*)buffer, size_of_file);
    block_size = encoded_size/MPI_size;
    for (int i = 1; i < MPI_size; i++) {
      if( i == MPI_size-1 ) {
	unsigned long long last_block_size = block_size + (encoded_size%MPI_size);
        MPI_Send(&last_block_size, 1, MPI_UNSIGNED_LONG_LONG, 
			i, 0, MPI_COMM_WORLD);
	MPI_Send(&buffer[block_size*i], 
			last_block_size, 
			MPI_UNSIGNED_CHAR,
			i, 0, MPI_COMM_WORLD);
	
      } else {
        MPI_Send(&block_size, 1, MPI_UNSIGNED_LONG_LONG, 
			i, 0, MPI_COMM_WORLD);
	MPI_Send(&buffer[block_size*i], 
			block_size, 
			MPI_UNSIGNED_CHAR,
			i, 0, MPI_COMM_WORLD);
      }
    }
  } else {
      MPI_Recv(&block_size, 1, MPI_UNSIGNED_LONG_LONG,
	       0, 0, MPI_COMM_WORLD, NULL);
      buffer = new unsigned char [block_size];
      MPI_Recv(&buffer[0], block_size, MPI_UNSIGNED_CHAR,
	       0, 0, MPI_COMM_WORLD, NULL);
  }
  std::vector<int> decoded_char, bit_idx;
  std::vector<unsigned long long> indexs;
  int last_len=0;
  unsigned long long int byte, bit=0;
  for (byte = 0; byte < block_size; byte++) {
    char nextByte = buffer[byte];
      for (bit = 0; bit < 8; bit++) {
        code += ((nextByte >> bit) & 0x01)?'1':'0';
        auto exist = codebook_map.find(code);
	if (exist != codebook_map.end()) {
	  int index = exist->second;
 	  if( MPI_rank == 0 ) {
	    output_file << (unsigned char)index;
	  } else {
	    indexs.push_back( (block_size*MPI_rank + byte) - (code.size())/8 );
	    bit_idx.push_back(  (bit - code.size())%8 );
	    decoded_char.push_back(index);
	    last_len = code.size();
	  }
	  code.clear();
	}
      }
  }
  if (MPI_rank==0) {
	  /* iterate all rank and find synchro */
	for( int i = 1; i < MPI_size; i++ ) {
		bool syc = false;
		int size;
		MPI_Recv(&size, 1, MPI_INT,
				i, 0, MPI_COMM_WORLD, NULL);
		indexs.resize(size);
		MPI_Recv(&indexs[0], size, MPI_UNSIGNED_LONG_LONG,
				i, 0, MPI_COMM_WORLD, NULL);
		decoded_char.resize(size);
		MPI_Recv(&decoded_char[0], size, MPI_INT,
				i, 0, MPI_COMM_WORLD, NULL);
		bit_idx.resize(size);
		MPI_Recv(&bit_idx[0], size, MPI_INT,
				i, 0, MPI_COMM_WORLD, NULL);
		MPI_Recv(&last_len, 1, MPI_INT,
				i, 0, MPI_COMM_WORLD, NULL);
		unsigned long long end = std::min(block_size*(i+1), (unsigned long long )encoded_size);
		unsigned long long ptr = 0;
		for ( ; byte < end; byte++ ) {
		   char nextByte = buffer[byte];
		   bit%=8;
		   for (; bit < 8; bit ++ ) {
		     code += ((nextByte >> bit) & 0x01)?'1':'0';
		     auto exist = codebook_map.find(code);
		     if (exist != codebook_map.end()) {
		         int index = exist->second;
		         output_file << (unsigned char)index;
		         code.clear();
			 while ( ptr < indexs.size() && indexs[ptr] < byte)  ptr++;
			 if (  ptr < indexs.size() && indexs[ptr] == byte && bit_idx[ptr] == (bit+1) ) {
				 int k;
				 for (k = ptr; k < size; k++ ) {
					 output_file << (unsigned char) decoded_char[k];
				 }
			         nextByte = buffer[byte];
				 byte= indexs[k-1];
				 bit = bit_idx[k-1];
				 byte += last_len/8;
				 bit  += last_len%8 + 1;
				 if(bit/8) byte++;
				 code.clear();
			     
				 syc = true;
				 break;
			 }
		     }
		   }
		   if(syc) break;
		}
	}
  } else {
	  /* send bit index and decode to rank 0 */
	  int size = indexs.size();
	  MPI_Send(&size, 1, MPI_INT,
			  0, 0, MPI_COMM_WORLD);
	  MPI_Send(&indexs[0], size, MPI_UNSIGNED_LONG_LONG,
			  0, 0, MPI_COMM_WORLD);
	  MPI_Send(&decoded_char[0], size, MPI_INT,
			  0, 0, MPI_COMM_WORLD);
	  MPI_Send(&bit_idx[0], size, MPI_INT,
			  0, 0, MPI_COMM_WORLD);
	  MPI_Send(&last_len, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
  }
  MPI_Finalize();
  output_file.close();
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
