CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -march=native
LDFLAGS  :=

TARGETS  := freespit

.PHONY: all clean

all: $(TARGETS)

freespit: freespit.cc
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGETS)
