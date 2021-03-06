CXX=g++
CFLAGS=-std=c++14 -Iinc -I$(VULKAN_SDK)/include -Wall -g
LFLAGS=-lSDL2 -lvulkan -Wall -g
BIN=vulkan
SRC=$(wildcard src/*.cpp)
OBJ=$(patsubst src/%,obj/%,$(patsubst %.cpp,%.o,$(SRC)))

default: $(BIN)

obj/%.o: src/%.cpp
	$(CXX) -o $@ -c $(CFLAGS) $<

$(BIN): $(OBJ)
	$(CXX) $(LFLAGS) -o $(BIN) $(OBJ)

.PHONY=clean run

run: $(BIN)
	./$(BIN)

clean:
	rm $(BIN) obj/*
