CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra
BIN      := bin

SOLVERS  := $(patsubst src/solvers/%.cpp,$(BIN)/%,$(wildcard src/solvers/*.cpp))
TOOLS    := $(patsubst src/tools/%.cpp,$(BIN)/%,$(wildcard src/tools/*.cpp))
HEADERS  := $(wildcard src/*.hpp)

all: $(BIN)/parser $(SOLVERS) $(TOOLS)

$(BIN)/parser: parser.cpp $(HEADERS) | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BIN)/%: src/solvers/%.cpp $(HEADERS) | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BIN)/%: src/tools/%.cpp $(HEADERS) | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BIN):
	mkdir -p $(BIN) output

clean:
	rm -rf $(BIN)

.PHONY: all clean
