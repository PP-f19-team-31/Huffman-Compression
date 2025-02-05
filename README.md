# Parallel Huffman

Takes input and provides compressed output via the Parallel Huffman encoding algorithm

To build:
    - Open a terminal
    - Go to folder with files & makefile
    - type `make`

To run:
    - In terminal
      - to compress
        - type: `huffman -c notcompressed.txt -o compressed.txt`
      - to decompress
        - type: `huffman -d compressed.txt -o decompressed.txt`

## example:
```
huffman -c Huckleberry.txt -o Huckleberry_compressed.txt
huffman -c Huckleberry_compressed.txt -o Huckleberry.2.txt
```

## Measure the execution time of each part
```
make CXXFLAGS="-DDEBUG -O3 -Wall -g -fopenmp -mavx"
```


### Compress
```
make compress
```


### Decompress
```
make decompress
```

### Test Equality
```
make test
```
