CXX = g++
CXXFLAGS += -O3 -Wall -g -fopenmp -mavx
LDFALGS += -lpthread -fopenmp
SRC = $(wildcard *.cpp)
OBJ = $(SRC:%.cpp=%.o)
TARGET = huffman
TEST = HuckleBerry
FORMATER = clang-format -i

all: $(SRC) $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(LDFALGS) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

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

clean :
	$(RM) $(OBJ) $(TARGET) $(TEST).2.txt $(TEST).compress.txt
