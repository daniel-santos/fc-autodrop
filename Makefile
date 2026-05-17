CXX      := g++
CXXFLAGS := -std=c++17 -O2 -march=native
CXXFLAGS := -std=c++17 -O0 -march=native -ggdb3
WARNFLAGS:= -Wall -Wextra
CXXFLAGS += $(WARNFLAGS)
LDFLAGS  :=

TARGETS  := freespit

.PHONY: all clean

all: $(TARGETS)

freespit: freespit.cc
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGETS)
