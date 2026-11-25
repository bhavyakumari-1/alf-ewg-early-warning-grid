# ALF-EWG build file (works with g++ / MinGW / clang++)
CXX      ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -Wpedantic -O2 -Iinclude
SRC      := $(wildcard src/*.cpp)
OBJ      := $(SRC:src/%.cpp=build/%.o)
TARGET   := alf_ewg

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

run: $(TARGET)
	./$(TARGET)

# memory-safety build (AddressSanitizer + UBSan) used to verify there are no leaks
sanitize:
	$(CXX) -std=c++11 -g -fsanitize=address,undefined -Iinclude $(SRC) -o alf_ewg_asan
	./alf_ewg_asan > /dev/null && echo "Sanitizer run: no leaks / no undefined behaviour"

clean:
	rm -rf build $(TARGET) alf_ewg_asan *.bin stations_snapshot.txt

.PHONY: all run sanitize clean
