CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra
BIN      := bin

# The solvers in src/solvers/ have no main(): parser.cpp includes them all
HEADERS  := $(wildcard src/*.hpp)
SOLVERS  := $(wildcard src/solvers/*.cpp)

all: $(BIN)/parser $(BIN)/judge

$(BIN)/parser: parser.cpp $(HEADERS) $(SOLVERS) | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $<

# Official HashCode judge, third-party code: built without warnings
$(BIN)/judge: src/tools/judge.cpp | $(BIN)
	$(CXX) $(CXXFLAGS) -w -o $@ $<

$(BIN):
	mkdir -p $(BIN)

clean:
	rm -rf $(BIN)

.PHONY: all clean
