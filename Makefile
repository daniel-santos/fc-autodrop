CXX      := g++
CXXFLAGS := -std=c++17 -O0 -march=native -ggdb3
CXXFLAGS := -std=c++17 -O3 -fno-tree-vectorize -march=native -ggdb3
CXXFLAGS := -std=c++17 -O2 -march=native -ggdb3
WARNFLAGS:= -Wall -Wextra -Wnon-virtual-dtor
CXXFLAGS += $(WARNFLAGS)
LDFLAGS  :=

TARGETS  := fc-autodrop

.PHONY: all clean

all: $(TARGETS)

fc-autodrop: fc-autodrop.cc
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGETS)
