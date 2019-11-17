CXX = g++
CXXFLAGS += -O3 -Wall -g
LDFALGS += -lpthread
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

test: all
	./$(TARGET) -c $(TEST).txt -o $(TEST).compress.txt
	./$(TARGET) -d $(TEST).compress.txt -o $(TEST).2.txt
	diff $(TEST).txt $(TEST).2.txt 

clean : 
	$(RM) $(OBJ) $(TARGET) $(TEST).2.txt $(TEST).compress.txt
