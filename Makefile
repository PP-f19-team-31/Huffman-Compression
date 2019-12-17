CXX = g++
MPI = /home/PP-f19/MPI/bin/mpic++
MPIEXE = /home/PP-f19/MPI/bin/mpiexec
CXXFLAGS += -O3 -Wall -g -fopenmp
LDFALGS += -lpthread -fopenmp
SRC = $(wildcard *.cpp)
OBJ = $(SRC:%.cpp=%.o)
TARGET = huffman
TEST = data32M
FORMATER = clang-format -i

all: $(OBJ) $(TARGET) 

$(TARGET): $(OBJ)
	$(MPI) $(LDFALGS) $^ -o $@

%.o: %.cpp
	$(MPI) $(CXXFLAGS) -c $< -o $@

format:
	$(FORMATER) $(SRC)

compress: all
	time --format="executime time: %E" ./$(TARGET) -c $(TEST).txt -o $(TEST).compress.txt

decompress: all
	time --format="executime time: %E" ./$(TARGET) -d $(TEST).compress.txt -o $(TEST).2.txt

test: all
	time --format="executime time: %E" ./$(TARGET) -c $(TEST).txt -o $(TEST).compress.txt
	time --format="executime time: %E" ./$(TARGET) -d $(TEST).compress.txt -o $(TEST).2.txt
	diff $(TEST).txt $(TEST).2.txt

perf: all
	perf record --call-graph dwarf ./$(TARGET) -c $(TEST).txt -o $(TEST).compress.txt
mpi: all
	./$(TARGET) -c $(TEST).txt -o $(TEST).compress.txt
	time --format="exe time: %E" $(MPIEXE) --npernode 4 --hostfile hostfile $(TARGET) -d $(TEST).compress.txt -o $(TEST)_decompress.txt
	diff $(TEST).txt $(TEST)_decompress.txt
clean :
	$(RM) $(OBJ) $(TARGET) $(TEST).2.txt $(TEST).compress.txt $(TEST)_decompress.txt
